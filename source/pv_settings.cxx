/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pv_settings.hxx"

#include "ftn2xml/auxiliary.hxx"

extern GeanyData *geany_data;

void PreviewSettings::initialize() {
  add_setting((TkuiSetting *)&startup_timeout, TKUI_PREF_TYPE_INTEGER,
              "startup_timeout", desc_startup_timeout, false);

  add_setting((TkuiSetting *)&update_interval_slow, TKUI_PREF_TYPE_INTEGER,
              "update_interval_slow", desc_update_interval_slow, false);
  add_setting((TkuiSetting *)&size_factor_slow, TKUI_PREF_TYPE_DOUBLE,
              "size_factor_slow", desc_size_factor_slow, false);
  add_setting((TkuiSetting *)&update_interval_fast, TKUI_PREF_TYPE_INTEGER,
              "update_interval_fast", desc_update_interval_fast, false);
  add_setting((TkuiSetting *)&size_factor_fast, TKUI_PREF_TYPE_DOUBLE,
              "size_factor_fast", {}, false);

  add_setting((TkuiSetting *)&default_type, TKUI_PREF_TYPE_STRING,
              "default_type", desc_default_type, false);

  add_setting((TkuiSetting *)&processor_html, TKUI_PREF_TYPE_STRING,
              "processor_html", desc_processor_html, false);
  add_setting((TkuiSetting *)&processor_markdown, TKUI_PREF_TYPE_STRING,
              "processor_markdown", desc_processor_markdown, false);
  add_setting((TkuiSetting *)&processor_asciidoc, TKUI_PREF_TYPE_STRING,
              "processor_asciidoc", desc_processor_asciidoc, false);
  add_setting((TkuiSetting *)&processor_fountain, TKUI_PREF_TYPE_STRING,
              "processor_fountain", desc_processor_fountain, false);
  add_setting((TkuiSetting *)&wiki_default, TKUI_PREF_TYPE_STRING,
              "wiki_default", desc_wiki_default, false);

  add_setting((TkuiSetting *)&pandoc_disabled, TKUI_PREF_TYPE_BOOLEAN,
              "pandoc_disabled", desc_pandoc_disabled, false);
  add_setting((TkuiSetting *)&pandoc_fragment, TKUI_PREF_TYPE_BOOLEAN,
              "pandoc_fragment", desc_pandoc_fragment, false);
  add_setting((TkuiSetting *)&pandoc_toc, TKUI_PREF_TYPE_BOOLEAN, "pandoc_toc",
              desc_pandoc_toc, false);

  add_setting((TkuiSetting *)&pandoc_markdown, TKUI_PREF_TYPE_STRING,
              "pandoc_markdown", desc_pandoc_markdown, false);
  add_setting((TkuiSetting *)&default_font_family, TKUI_PREF_TYPE_STRING,
              "default_font_family", desc_default_font_family, false);

  add_setting((TkuiSetting *)&extended_types, TKUI_PREF_TYPE_BOOLEAN,
              "extended_types", desc_extended_types, false);

  add_setting((TkuiSetting *)&snippet_window, TKUI_PREF_TYPE_INTEGER,
              "snippet_window", desc_snippet_window, false);
  add_setting((TkuiSetting *)&snippet_trigger, TKUI_PREF_TYPE_INTEGER,
              "snippet_trigger", {}, false);

  add_setting((TkuiSetting *)&snippet_html, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_html", desc_snippet_html, false);
  add_setting((TkuiSetting *)&snippet_markdown, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_markdown", {}, false);
  add_setting((TkuiSetting *)&snippet_asciidoctor, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_asciidoctor", {}, false);
  add_setting((TkuiSetting *)&snippet_pandoc, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_pandoc", {}, false);
  add_setting((TkuiSetting *)&snippet_fountain, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_fountain", {}, false);

  add_setting((TkuiSetting *)&extra_css, TKUI_PREF_TYPE_STRING, "extra_css",
              desc_extra_css, false);
}

void PreviewSettings::load() {
  if (!keyfile) {
    kf_open();
  }

  g_key_file_load_from_file(keyfile, config_file.c_str(),
                            GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS), nullptr);

  if (!g_key_file_has_group(keyfile, PREVIEW_KF_GROUP)) {
    return;
  }

  for (auto pref : pref_entries) {
    switch (pref.type) {
      case TKUI_PREF_TYPE_BOOLEAN:
        if (kf_has_key(pref.name)) {
          *(bool *)pref.setting = g_key_file_get_boolean(
              keyfile, PREVIEW_KF_GROUP, pref.name.c_str(), nullptr);
        }
        break;
      case TKUI_PREF_TYPE_INTEGER:
        if (kf_has_key(pref.name)) {
          *(int *)pref.setting = g_key_file_get_integer(
              keyfile, PREVIEW_KF_GROUP, pref.name.c_str(), nullptr);
        }
        break;
      case TKUI_PREF_TYPE_DOUBLE:
        if (kf_has_key(pref.name)) {
          *(double *)pref.setting = g_key_file_get_double(
              keyfile, PREVIEW_KF_GROUP, pref.name.c_str(), nullptr);
        }
        break;
      case TKUI_PREF_TYPE_STRING:
        if (kf_has_key(pref.name)) {
          char *val = g_key_file_get_string(keyfile, PREVIEW_KF_GROUP,
                                            pref.name.c_str(), nullptr);
          *(std::string *)pref.setting = cstr_assign(val);
        }
        break;
      default:
        break;
    }
  }
}

void PreviewSettings::save_session() { save(true); }

void PreviewSettings::save(bool bSession) {
  // Load old contents in case user changed file outside of GUI
  g_key_file_load_from_file(keyfile, config_file.c_str(),
                            GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS), nullptr);

  for (auto pref : pref_entries) {
    if (bSession && !pref.session) {
      continue;
    }

    switch (pref.type) {
      case TKUI_PREF_TYPE_BOOLEAN:
        g_key_file_set_boolean(keyfile, PREVIEW_KF_GROUP, pref.name.c_str(),
                               *reinterpret_cast<bool *>(pref.setting));
        break;
      case TKUI_PREF_TYPE_INTEGER:
        g_key_file_set_integer(keyfile, PREVIEW_KF_GROUP, pref.name.c_str(),
                               *reinterpret_cast<int *>(pref.setting));
        break;
      case TKUI_PREF_TYPE_DOUBLE:
        g_key_file_set_double(keyfile, PREVIEW_KF_GROUP, pref.name.c_str(),
                              *reinterpret_cast<double *>(pref.setting));
        break;
      case TKUI_PREF_TYPE_STRING:
        g_key_file_set_string(
            keyfile, PREVIEW_KF_GROUP, pref.name.c_str(),
            reinterpret_cast<std::string *>(pref.setting)->c_str());
        break;
      default:
        break;
    }
    if (!pref.comment.empty()) {
      g_key_file_set_comment(keyfile, PREVIEW_KF_GROUP, pref.name.c_str(),
                             (" " + pref.comment).c_str(), nullptr);
    }
  }

  // Store back on disk
  std::string contents =
      cstr_assign(g_key_file_to_data(keyfile, nullptr, nullptr));
  if (!contents.empty()) {
    file_set_contents(config_file, contents);
  }
}

void PreviewSettings::reset() {
  if (config_file.empty()) {
    kf_open();
  }

  {  // delete if file exists
    GFile *file = g_file_new_for_path(config_file.c_str());
    if (!g_file_trash(file, nullptr, nullptr)) {
      g_file_delete(file, nullptr, nullptr);
    }
    g_object_unref(file);
  }

  PreviewSettings new_settings;
  new_settings.initialize();
  new_settings.kf_open();
  new_settings.save();
  new_settings.kf_close();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void PreviewSettings::kf_open() {
  keyfile = g_key_file_new();

  config_file =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", nullptr));
  std::string conf_dn = g_path_get_dirname(config_file.c_str());
  g_mkdir_with_parents(conf_dn.c_str(), 0755);

  // if file does not exist, create it
  if (!g_file_test(config_file.c_str(), G_FILE_TEST_EXISTS)) {
    file_set_contents(config_file, "[" PREVIEW_KF_GROUP "]");
    save();
  }
}

void PreviewSettings::kf_close() {
  if (keyfile) {
    g_key_file_free(keyfile);
    keyfile = nullptr;
  }
}

bool PreviewSettings::kf_has_key(const std::string &key) {
  return g_key_file_has_key(keyfile, PREVIEW_KF_GROUP, key.c_str(), nullptr);
}

void PreviewSettings::add_setting(TkuiSetting *setting,
                                  const TkuiSettingType &type,
                                  const std::string &name,
                                  const std::string &comment,
                                  const bool &session) {
  TkuiSettingPref pref{type, name, comment, session, setting};
  pref_entries.push_back(pref);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
