/* -*- C++ -*-
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
#include "prefs.h"

// Functions

void PreviewSettings::open() {
  std::string conf_fn =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", nullptr));
  std::string conf_dn = cstr_assign(g_path_get_dirname(conf_fn.c_str()));
  g_mkdir_with_parents(conf_dn.c_str(), 0755);

  GKeyFile *kf = g_key_file_new();

  // if file does not exist, create it
  if (!g_file_test(conf_fn.c_str(), G_FILE_TEST_EXISTS)) {
    save_default();
  }

  g_key_file_load_from_file(
      kf, conf_fn.c_str(),
      GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
      nullptr);

  load(kf);

  GKEY_FILE_FREE(kf);
}

void PreviewSettings::save_default() {
  static std::string conf_fn =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", nullptr));
  std::string conf_dn = cstr_assign(g_path_get_dirname(conf_fn.c_str()));
  g_mkdir_with_parents(conf_dn.c_str(), 0755);

  // delete if file exists
  GFile *file = g_file_new_for_path(conf_fn.c_str());
  if (!g_file_trash(file, nullptr, nullptr)) {
    g_file_delete(file, nullptr, nullptr);
  }

  // copy default config
  std::string contents = file_get_contents(PREVIEW_CONFIG);
  if (!contents.empty()) {
    file_set_contents(conf_fn, contents);
  }

  g_object_unref(file);
}

void PreviewSettings::save() {
  GKeyFile *kf = g_key_file_new();
  std::string conf_fn =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", nullptr));

  // Load old contents in case user changed file outside of GUI
  g_key_file_load_from_file(
      kf, conf_fn.c_str(),
      GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
      nullptr);

  // Update settings with new contents
  SET_KEY(integer, "update_interval_slow", update_interval_slow);
  SET_KEY(double, "size_factor_slow", size_factor_slow);
  SET_KEY(integer, "update_interval_fast", update_interval_fast);
  SET_KEY(double, "size_factor_fast", size_factor_fast);

  SET_KEY_STRING("html_processor", html_processor);
  SET_KEY_STRING("markdown_processor", markdown_processor);
  SET_KEY_STRING("asciidoc_processor", asciidoc_processor);
  SET_KEY_STRING("fountain_processor", fountain_processor);
  SET_KEY_STRING("wiki_default", wiki_default);

  SET_KEY(boolean, "pandoc_disabled", pandoc_disabled);
  SET_KEY(boolean, "pandoc_fragment", pandoc_fragment);
  SET_KEY(boolean, "pandoc_toc", pandoc_toc);
  SET_KEY_STRING("pandoc_markdown", pandoc_markdown);
  SET_KEY_STRING("default_font_family", default_font_family);

  SET_KEY(boolean, "extended_types", extended_types);

  SET_KEY(integer, "snippet_window", snippet_window);
  SET_KEY(integer, "snippet_trigger", snippet_trigger);

  SET_KEY(boolean, "snippet_html", snippet_html);
  SET_KEY(boolean, "snippet_markdown", snippet_markdown);
  SET_KEY(boolean, "snippet_asciidoctor", snippet_asciidoctor);
  SET_KEY(boolean, "snippet_pandoc", snippet_pandoc);
  SET_KEY(boolean, "snippet_screenplain", snippet_screenplain);

  SET_KEY_STRING("extra_css", extra_css);

  // Store back on disk
  std::string contents = cstr_assign(g_key_file_to_data(kf, nullptr, nullptr));
  if (!contents.empty()) {
    file_set_contents(conf_fn, contents);
  }

  GKEY_FILE_FREE(kf);
}

void PreviewSettings::load(GKeyFile *kf) {
  if (!kf || !g_key_file_has_group(kf, "preview")) {
    return;
  }

  GET_KEY_INTEGER(update_interval_slow, 200, 5);
  GET_KEY_DOUBLE(size_factor_slow, 0.004, 0);
  GET_KEY_INTEGER(update_interval_fast, 25, 5);
  GET_KEY_DOUBLE(size_factor_fast, 0.001, 0);

  GET_KEY_STRING(html_processor, "native");
  GET_KEY_STRING(markdown_processor, "native");
  GET_KEY_STRING(asciidoc_processor, "asciidoctor");
  GET_KEY_STRING(fountain_processor, "screenplain");
  GET_KEY_STRING(wiki_default, "mediawiki");

  GET_KEY_BOOLEAN(pandoc_disabled, false);
  GET_KEY_BOOLEAN(pandoc_fragment, false);
  GET_KEY_BOOLEAN(pandoc_toc, false);

  GET_KEY_STRING(pandoc_markdown, "markdown");
  GET_KEY_STRING(default_font_family, "serif");

  GET_KEY_BOOLEAN(extended_types, true);

  GET_KEY_INTEGER(snippet_window, 5000, 5);
  GET_KEY_INTEGER(snippet_trigger, 100000, 5);

  GET_KEY_BOOLEAN(snippet_html, false);
  GET_KEY_BOOLEAN(snippet_markdown, true);
  GET_KEY_BOOLEAN(snippet_asciidoctor, true);
  GET_KEY_BOOLEAN(snippet_pandoc, true);
  GET_KEY_BOOLEAN(snippet_screenplain, true);

  GET_KEY_STRING(extra_css, "disabled");
}
