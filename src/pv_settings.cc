/*
 * Preview Geany Plugin
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

// Functions

PreviewSettings::~PreviewSettings() { close(); }

void PreviewSettings::open() {
  config_file =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", nullptr));
  std::string conf_dn = g_path_get_dirname(config_file.c_str());
  g_mkdir_with_parents(conf_dn.c_str(), 0755);

  // if file does not exist, create it
  if (!g_file_test(config_file.c_str(), G_FILE_TEST_EXISTS)) {
    reset();
  }

  keyfile = g_key_file_new();
}

void PreviewSettings::close() {
  if (keyfile) {
    g_key_file_free(keyfile);
    keyfile = nullptr;
  }
}

void PreviewSettings::reset() {
  if (config_file.empty()) {
    open();
  }

  {  // delete if file exists
    GFile *file = g_file_new_for_path(config_file.c_str());
    if (!g_file_trash(file, nullptr, nullptr)) {
      g_file_delete(file, nullptr, nullptr);
    }
    g_object_unref(file);
  }

  // copy default config
  std::string contents = file_get_contents(PREVIEW_CONFIG);
  if (!contents.empty()) {
    file_set_contents(config_file, contents);
  }
}

void PreviewSettings::save() {
  if (!keyfile) {
    return;
  }

  bSaveInProgress = true;

  // Load old contents in case user changed file outside of GUI
  g_key_file_load_from_file(
      keyfile, config_file.c_str(),
      GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
      nullptr);

  // Update settings with new contents

  kf_set_integer("update_interval_slow", update_interval_slow);
  kf_set_double("size_factor_slow", size_factor_slow);
  kf_set_integer("update_interval_fast", update_interval_fast);
  kf_set_double("size_factor_fast", size_factor_fast);

  kf_set_string("html_processor", html_processor);
  kf_set_string("markdown_processor", markdown_processor);
  kf_set_string("asciidoc_processor", asciidoc_processor);
  kf_set_string("fountain_processor", fountain_processor);
  kf_set_string("wiki_default", wiki_default);

  kf_set_boolean("pandoc_disabled", pandoc_disabled);

  kf_set_boolean("pandoc_fragment", pandoc_fragment);
  kf_set_boolean("pandoc_toc", pandoc_toc);
  kf_set_string("pandoc_markdown", pandoc_markdown);
  kf_set_string("default_font_family", default_font_family);

  kf_set_boolean("extended_types", extended_types);

  kf_set_integer("snippet_window", snippet_window);
  kf_set_integer("snippet_trigger", snippet_trigger);

  kf_set_boolean("snippet_html", snippet_html);
  kf_set_boolean("snippet_markdown", snippet_markdown);
  kf_set_boolean("snippet_asciidoctor", snippet_asciidoctor);
  kf_set_boolean("snippet_pandoc", snippet_pandoc);
  kf_set_boolean("snippet_screenplain", snippet_screenplain);

  kf_set_string("extra_css", extra_css);

  save_session();
}

void PreviewSettings::save_session() {
  if (!keyfile) {
    return;
  }

  if (!bSaveInProgress) {
    // Load old contents in case user changed file outside of GUI
    g_key_file_load_from_file(
        keyfile, config_file.c_str(),
        GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
        nullptr);
  }

  // Preview doesn't currently use any session info

  // Store back on disk
  std::string contents =
      cstr_assign(g_key_file_to_data(keyfile, nullptr, nullptr));
  if (!contents.empty()) {
    file_set_contents(config_file, contents);
  }

  bSaveInProgress = false;
}

void PreviewSettings::load() {
  if (!keyfile) {
    return;
  }

  g_key_file_load_from_file(
      keyfile, config_file.c_str(),
      GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
      nullptr);

  if (!g_key_file_has_group(keyfile, PREVIEW_KF_GROUP)) {
    return;
  }

  update_interval_slow = kf_get_integer("update_interval_slow", 200, 5);
  size_factor_slow = kf_get_double("size_factor_slow", 0.004, 0);
  update_interval_fast = kf_get_integer("update_interval_fast", 25, 5);
  size_factor_fast = kf_get_double("size_factor_fast", 0.001, 0);

  html_processor = kf_get_string("html_processor", "native");
  markdown_processor = kf_get_string("markdown_processor", "native");
  asciidoc_processor = kf_get_string("asciidoc_processor", "asciidoctor");
  fountain_processor = kf_get_string("fountain_processor", "screenplain");
  wiki_default = kf_get_string("wiki_default", "mediawiki");

  pandoc_disabled = kf_get_boolean("pandoc_disabled", false);
  pandoc_fragment = kf_get_boolean("pandoc_fragment", false);
  pandoc_toc = kf_get_boolean("pandoc_toc", false);

  pandoc_markdown = kf_get_string("pandoc_markdown", "markdown");
  default_font_family = kf_get_string("default_font_family", "serif");

  extended_types = kf_get_boolean("extended_types", true);

  snippet_window = kf_get_integer("snippet_window", 5000, 5);
  snippet_trigger = kf_get_integer("snippet_trigger", 100000, 5);

  snippet_html = kf_get_boolean("snippet_html", false);
  snippet_markdown = kf_get_boolean("snippet_markdown", true);
  snippet_asciidoctor = kf_get_boolean("snippet_asciidoctor", true);
  snippet_pandoc = kf_get_boolean("snippet_pandoc", true);
  snippet_screenplain = kf_get_boolean("snippet_screenplain", true);

  extra_css = kf_get_string("extra_css", "disabled");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Keyfile operations
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool PreviewSettings::kf_has_key(std::string const &key) {
  return g_key_file_has_key(keyfile, PREVIEW_KF_GROUP, key.c_str(), nullptr);
}

void PreviewSettings::kf_set_boolean(std::string const &key, bool const &val) {
  g_key_file_set_boolean(keyfile, PREVIEW_KF_GROUP, key.c_str(), val);
}

bool PreviewSettings::kf_get_boolean(std::string const &key, bool const &def) {
  if (kf_has_key(key)) {
    return g_key_file_get_boolean(keyfile, PREVIEW_KF_GROUP, key.c_str(),
                                  nullptr);
  } else {
    return def;
  }
}

void PreviewSettings::kf_set_double(std::string const &key, double const &val) {
  g_key_file_set_double(keyfile, PREVIEW_KF_GROUP, key.c_str(), val);
}

double PreviewSettings::kf_get_double(std::string const &key, double const &def,
                                   double const &min) {
  if (kf_has_key(key)) {
    int val =
        g_key_file_get_double(keyfile, PREVIEW_KF_GROUP, key.c_str(), nullptr);

    if (val < min) {
      return min;
    } else {
      return val;
    }
  } else {
    return def;
  }
}

void PreviewSettings::kf_set_integer(std::string const &key, int const &val) {
  g_key_file_set_integer(keyfile, PREVIEW_KF_GROUP, key.c_str(), val);
}

int PreviewSettings::kf_get_integer(std::string const &key, int const &def,
                                    int const &min) {
  if (kf_has_key(key)) {
    int val =
        g_key_file_get_integer(keyfile, PREVIEW_KF_GROUP, key.c_str(), nullptr);

    if (val < min) {
      return min;
    } else {
      return val;
    }
  } else {
    return def;
  }
}

void PreviewSettings::kf_set_string(std::string const &key,
                                    std::string const &val) {
  g_key_file_set_string(keyfile, PREVIEW_KF_GROUP, key.c_str(), val.c_str());
}

std::string PreviewSettings::kf_get_string(std::string const &key,
                                           std::string const &def) {
  if (kf_has_key(key)) {
    char *val =
        g_key_file_get_string(keyfile, PREVIEW_KF_GROUP, key.c_str(), nullptr);
    return cstr_assign(val);
  } else {
    return def;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
