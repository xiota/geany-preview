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

#ifndef PREVIEW_PREFS_H
#define PREVIEW_PREFS_H

#include "plugin.h"

G_BEGIN_DECLS

class PreviewSettings {
 public:
  PreviewSettings() {
    html_processor = g_strdup("native");
    markdown_processor = g_strdup("native");
    asciidoc_processor = g_strdup("asciidoctor");
    fountain_processor = g_strdup("screenplain");
    wiki_default = g_strdup("mediawiki");
    pandoc_markdown = g_strdup("markdown");
    default_font_family = g_strdup("serif");
    extra_css = g_strdup("disabled");
  }

 public:
  int update_interval_slow = 200;
  double size_factor_slow = 0.004;
  int update_interval_fast = 25;
  double size_factor_fast = 0.001;
  char *html_processor = nullptr;
  char *markdown_processor = nullptr;
  char *asciidoc_processor = nullptr;
  char *fountain_processor = nullptr;
  char *wiki_default = nullptr;
  bool pandoc_disabled = false;
  bool pandoc_fragment = false;
  bool pandoc_toc = false;
  char *pandoc_markdown = nullptr;
  char *default_font_family = nullptr;
  bool extended_types = true;
  int snippet_window = 5000;
  int snippet_trigger = 100000;
  bool snippet_html = false;
  bool snippet_markdown = true;
  bool snippet_asciidoctor = true;
  bool snippet_pandoc = true;
  bool snippet_screenplain = true;
  char *extra_css = nullptr;
};

void open_settings();
void load_settings(GKeyFile *kf);
void save_settings();
void save_default_settings();

G_END_DECLS

// Macros to make loading settings easier
#define PLUGIN_GROUP "preview"

#define HAS_KEY(key) g_key_file_has_key(kf, PLUGIN_GROUP, (key), nullptr)
#define GET_KEY(T, key) g_key_file_get_##T(kf, PLUGIN_GROUP, (key), nullptr)
#define SET_KEY(T, key, _val) \
  g_key_file_set_##T(kf, PLUGIN_GROUP, (key), (_val))

#define LOAD_KEY_STRING(key, def)        \
  do {                                   \
    if (HAS_KEY(#key)) {                 \
      char *val = GET_KEY(string, #key); \
      if (val) {                         \
        settings.key = g_strdup(val);    \
      } else {                           \
        settings.key = g_strdup((def));  \
      }                                  \
      g_free(val);                       \
    }                                    \
  } while (0)

#define LOAD_KEY_BOOLEAN(key, def)           \
  do {                                       \
    if (HAS_KEY(#key)) {                     \
      settings.key = GET_KEY(boolean, #key); \
    } else {                                 \
      settings.key = (def);                  \
    }                                        \
  } while (0)

#define LOAD_KEY_INTEGER(key, def, min) \
  do {                                  \
    if (HAS_KEY(#key)) {                \
      int val = GET_KEY(integer, #key); \
      if (val) {                        \
        if (val < (min)) {              \
          settings.key = (min);         \
        } else {                        \
          settings.key = val;           \
        }                               \
      } else {                          \
        settings.key = (def);           \
      }                                 \
    }                                   \
  } while (0)

#define LOAD_KEY_DOUBLE(key, def, min) \
  do {                                 \
    if (HAS_KEY(#key)) {               \
      int val = GET_KEY(double, #key); \
      if (val) {                       \
        if (val < (min)) {             \
          settings.key = (min);        \
        } else {                       \
          settings.key = val;          \
        }                              \
      } else {                         \
        settings.key = (def);          \
      }                                \
    }                                  \
  } while (0)

#endif  // PREVIEW_PREFS_H
