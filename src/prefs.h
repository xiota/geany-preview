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

struct PreviewSettings {
  int update_interval_slow;
  double size_factor_slow;
  int update_interval_fast;
  double size_factor_fast;
  char *html_processor;
  char *markdown_processor;
  char *asciidoc_processor;
  char *fountain_processor;
  char *wiki_default;
  bool pandoc_disabled;
  bool pandoc_fragment;
  bool pandoc_toc;
  char *pandoc_markdown;
  char *default_font_family;
  bool extended_types;
  int snippet_window;
  int snippet_trigger;
  bool snippet_html;
  bool snippet_markdown;
  bool snippet_asciidoctor;
  bool snippet_pandoc;
  bool snippet_screenplain;
  char *extra_css;
};

void init_settings();
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
