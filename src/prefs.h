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
  PreviewSettings() = default;
  ~PreviewSettings() = default;

  void open();
  void load(GKeyFile *kf);
  void save();
  void save_default();

 public:
  int update_interval_slow = 200;
  double size_factor_slow = 0.004;
  int update_interval_fast = 25;
  double size_factor_fast = 0.001;
  std::string html_processor{"native"};
  std::string markdown_processor{"native"};
  std::string asciidoc_processor{"asciidoctor"};
  std::string fountain_processor{"screenplain"};
  std::string wiki_default{"mediawiki"};
  bool pandoc_disabled = false;
  bool pandoc_fragment = false;
  bool pandoc_toc = false;
  std::string pandoc_markdown{"markdown"};
  std::string default_font_family{"serif"};
  bool extended_types = true;
  int snippet_window = 5000;
  int snippet_trigger = 100000;
  bool snippet_html = false;
  bool snippet_markdown = true;
  bool snippet_asciidoctor = true;
  bool snippet_pandoc = true;
  bool snippet_screenplain = true;
  std::string extra_css{"disabled"};
};

G_END_DECLS

// Macros to make loading settings easier
#define PLUGIN_GROUP "preview"

#define HAS_KEY(key) g_key_file_has_key(kf, PLUGIN_GROUP, (key), nullptr)
#define GET_KEY(T, key) g_key_file_get_##T(kf, PLUGIN_GROUP, (key), nullptr)
#define SET_KEY(T, key, _val) \
  g_key_file_set_##T(kf, PLUGIN_GROUP, (key), (_val))

#define SET_KEY_STRING(key, _val) \
  g_key_file_set_string(kf, PLUGIN_GROUP, (key), (_val.c_str()))

#define GET_KEY_STRING(key, def)         \
  do {                                   \
    if (HAS_KEY(#key)) {                 \
      char *val = GET_KEY(string, #key); \
      if (val) {                         \
        (key) = cstr_assign(val);        \
      } else {                           \
        (key) = std::string(def);        \
      }                                  \
    }                                    \
  } while (0)

#define GET_KEY_BOOLEAN(key, def)   \
  do {                              \
    if (HAS_KEY(#key)) {            \
      key = GET_KEY(boolean, #key); \
    } else {                        \
      key = (def);                  \
    }                               \
  } while (0)

#define GET_KEY_INTEGER(key, def, min)  \
  do {                                  \
    if (HAS_KEY(#key)) {                \
      int val = GET_KEY(integer, #key); \
      if (val) {                        \
        if (val < (min)) {              \
          key = (min);                  \
        } else {                        \
          key = val;                    \
        }                               \
      } else {                          \
        key = (def);                    \
      }                                 \
    }                                   \
  } while (0)

#define GET_KEY_DOUBLE(key, def, min)  \
  do {                                 \
    if (HAS_KEY(#key)) {               \
      int val = GET_KEY(double, #key); \
      if (val) {                       \
        if (val < (min)) {             \
          key = (min);                 \
        } else {                       \
          key = val;                   \
        }                              \
      } else {                         \
        key = (def);                   \
      }                                \
    }                                  \
  } while (0)

#endif  // PREVIEW_PREFS_H
