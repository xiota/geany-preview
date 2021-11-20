/*
 * Settings - Preview Plugin for Geany
 * Copyright 2021 xiota
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "auxiliary.h"
#include "pv_settings.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void PreviewSettings::initialize() {
  add_setting((TkuiSetting *)&startup_timeout, TKUI_PREF_TYPE_INTEGER,
              "startup_timeout",
              _("Timeout before forcing webview update during startup."),
              false);

  add_setting(
      (TkuiSetting *)&update_interval_slow, TKUI_PREF_TYPE_INTEGER,
      "update_interval_slow",
      _("The following option is the minimum number of milliseconds between "
        "calls for slow external programs.  This gives them time to finish "
        "processing.  Use a larger number for slower machines."),
      false);
  add_setting((TkuiSetting *)&size_factor_slow, TKUI_PREF_TYPE_DOUBLE,
              "size_factor_slow",
              _("For large files, a longer delay is needed.  The delay in "
                "milliseconds is the filesize in bytes multiplied by "
                "size_factor_slow."),
              false);
  add_setting((TkuiSetting *)&update_interval_fast, TKUI_PREF_TYPE_INTEGER,
              "update_interval_fast",
              _("Even fast programs may need a delay to finish processing "
                "on slow computers."),
              false);
  add_setting((TkuiSetting *)&size_factor_fast, TKUI_PREF_TYPE_DOUBLE,
              "size_factor_fast", {}, false);

  add_setting(
      (TkuiSetting *)&processor_html, TKUI_PREF_TYPE_STRING, "processor_html",
      _("The following option controls how HTML documents are processed.\n"
        "  - native* - Send the document directly to the webview.\n"
        "  - pandoc  - Use Pandoc to clean up the document and apply a "
        "stylesheet.\n"
        "  - disable - Turn off HTML previews."),
      false);
  add_setting(
      (TkuiSetting *)&processor_markdown, TKUI_PREF_TYPE_STRING,
      "processor_markdown",
      _("The following option selects the Markdown processor.\n"
        "  - native* - Use libcmark-gfm (faster)\n"
        "  - pandoc  - Use Pandoc GFM   (applies additional pandoc options)\n"
        "  - disable - Turn off Markdown previews."),
      false);
  add_setting((TkuiSetting *)&processor_asciidoc, TKUI_PREF_TYPE_STRING,
              "processor_asciidoc",
              _("The following option enables or disables AsciiDoc previews.\n"
                "  - asciidoctor* - Use asciidoctor.\n"
                "  - disable      - Turn off AsciiDoc previews."),
              false);
  add_setting((TkuiSetting *)&processor_fountain, TKUI_PREF_TYPE_STRING,
              "processor_fountain",
              _("The following option selects the Fountain processor.\n"
                "  - native*      - Use the built-in fountain processor\n"
                "  - screenplain  - Use screenplain (slower)\n"
                "  - disable      - Turn off Fountain screenplay previews"),
              false);
  add_setting(
      (TkuiSetting *)&wiki_default, TKUI_PREF_TYPE_STRING, "wiki_default",
      _("The following option selects the format that Pandoc uses to "
        "interpret files with the .wiki extension.  It can also turn off wiki "
        "previews.  Options may be any format from `pandoc "
        "--list-input-formats`: disable, dokuwiki, *mediawiki*, tikiwiki, "
        "twiki, vimwiki"),
      false);

  add_setting((TkuiSetting *)&pandoc_disabled, TKUI_PREF_TYPE_BOOLEAN,
              "pandoc_disabled",
              _("The following option disables Pandoc processing."), false);
  add_setting(
      (TkuiSetting *)&pandoc_fragment, TKUI_PREF_TYPE_BOOLEAN,
      "pandoc_fragment",
      _("The following option controls whether Pandoc produces standalone HTML "
        "files.  If set to true, stylesheets are effectively disabled."),
      false);
  add_setting((TkuiSetting *)&pandoc_toc, TKUI_PREF_TYPE_BOOLEAN, "pandoc_toc",
              _("The following option controls whether Pandoc inserts an "
                "auto-generated table of contents at the beginning of HTML "
                "files.  This might help with navigating long documents."),
              false);

  add_setting(
      (TkuiSetting *)&pandoc_markdown, TKUI_PREF_TYPE_STRING, "pandoc_markdown",
      _("The following option determines how Pandoc interprets Markdown.  It "
        "has *no* effect unless Pandoc is set as the processor_markdown.  "
        "Options may be any format from `pandoc --list-input-formats`: "
        "commonmark, *markdown*, markdown_github, markdown_mmd, "
        "markdown_phpextra, markdown_strict"),
      false);
  add_setting((TkuiSetting *)&default_font_family, TKUI_PREF_TYPE_STRING,
              "default_font_family",
              _("The following option determines what font family the webview "
                "uses when another font is not specified by a stylesheet."),
              false);

  add_setting(
      (TkuiSetting *)&extended_types, TKUI_PREF_TYPE_BOOLEAN, "extended_types",
      _("The following option controls whether the file extension is used to "
        "detect file types that are unknown to Geany.  This feature is needed "
        "to preview fountain, textile, wiki, muse, and org documents."),
      false);

  add_setting(
      (TkuiSetting *)&snippet_window, TKUI_PREF_TYPE_INTEGER, "snippet_window",
      _("Large documents are both slow to process and difficult to navigate.  "
        "The snippet settings allow a small region of interest to be "
        "previewed.  The trigger is the file size (bytes) that activates "
        "snippets.  The window is the size (bytes) around the caret that is "
        "processed."),
      false);
  add_setting((TkuiSetting *)&snippet_trigger, TKUI_PREF_TYPE_INTEGER,
              "snippet_trigger", {}, false);

  add_setting(
      (TkuiSetting *)&snippet_html, TKUI_PREF_TYPE_BOOLEAN, "snippet_html",
      _("Some document types don't work well with the snippets feature.  The "
        "following options disable (false) and enable (true) them."),
      false);
  add_setting((TkuiSetting *)&snippet_markdown, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_markdown", {}, false);
  add_setting((TkuiSetting *)&snippet_asciidoctor, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_asciidoctor", {}, false);
  add_setting((TkuiSetting *)&snippet_pandoc, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_pandoc", {}, false);
  add_setting((TkuiSetting *)&snippet_fountain, TKUI_PREF_TYPE_BOOLEAN,
              "snippet_fountain", {}, false);

  add_setting(
      (TkuiSetting *)&extra_css, TKUI_PREF_TYPE_STRING, "extra_css",
      _("Previews are styled with `css` files that correspond to the document "
        "processor and file type.  They are located in "
        "`~/.config/geany/plugins/preview/`\n"
        " Default files may be copied from "
        "`/usr/share/geany-plugins/preview/`\n"
        "\n"
        " Files are created as needed.  To disable, replace with an empty "
        "file.\n"
        "\n"
        "  - preview.css     - Includes rules for email-style headers.\n"
        "  - fountain.css    - Used by native fountain processor.\n"
        "  - markdown.css    - Used by the native markdown processor\n"
        "  - screenplain.css - Used by screenplain for fountain screenplays\n"
        "\n"
        "  - pandoc.css          - Used by Pandoc when a [format] file does "
        "not exist.\n"
        "  - pandoc-[format].css - Used by Pandoc for files written in "
        "[format]\n"
        "                          For example, pandoc-markdown.css\n"
        "\n"
        " The following option enables an extra, user-specified css file.  "
        "Included css files will be created when first used.\n"
        "  - extra-dark.css    - Dark theme.\n"
        "  - extra-invert.css  - Inverts everything.\n"
        "  - extra-media.css*  - @media rules to detect whether desktop uses "
        "dark theme.\n"
        "  - [filename]        - Create your own stylesheet\n"
        "  - disable           - Don't use any extra css files"),
      false);
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

bool PreviewSettings::kf_has_key(std::string const &key) {
  return g_key_file_has_key(keyfile, PREVIEW_KF_GROUP, key.c_str(), nullptr);
}

void PreviewSettings::add_setting(TkuiSetting *setting,
                                  TkuiSettingType const &type,
                                  std::string const &name,
                                  std::string const &comment,
                                  bool const &session) {
  TkuiSettingPref pref{type, name, comment, session, setting};
  pref_entries.push_back(pref);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
