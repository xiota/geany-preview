// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <gtk/gtk.h>

class PreviewConfig {
 public:
  // Expanded to support vectors of ints and strings
  using setting_value_t =
      std::variant<int, bool, std::string, std::vector<int>, std::vector<std::string> >;

  struct setting_def_t {
    const char *key;
    setting_value_t default_value;
    const char *help;
  };

  inline static const setting_def_t setting_defs_[] = {
      {"allow_update_fallback",
       setting_value_t{false},
       "Whether to use fallback rendering mode if DOM patch mode fails."                      },

      {"auto_set_read_only",
       setting_value_t{false},
       "Automatically set documents to read-only mode when they are not writable."            },

      {"column_markers",
       setting_value_t{false},
       "Enable or disable visual column markers in the editor."                               },

      {"column_markers_columns",
       setting_value_t{std::vector<int>{60, 72, 80, 88, 96, 104, 112, 120, 128}},
       "List of column positions (in characters) where vertical guide lines will be drawn."   },

      {"column_markers_colors",
       setting_value_t{std::vector<std::string>{
           "#e5e5e5",
           "#b0d0ff",
           "#ffc0ff",
           "#e5e5e5",
           "#ffb0a0",
           "#e5e5e5",
           "#e5e5e5",
           "#e5e5e5",
           "#e5e5e5"
       }},
       "Colors for each column marker, in #RRGGBB or #RGB format, matching the order of "
       "'column_markers_columns'."                                                            },

      {"update_cooldown",
       setting_value_t{65},
       "Time in milliseconds to wait between preview updates to prevent excessive refreshing."},

      {"update_min_delay",
       setting_value_t{15},
       "Time in milliseconds to wait before the first preview update after a change."         },

      {"terminal_command",
       setting_value_t{std::string{"xdg-terminal-exec --working-directory=%d"}},
       "Command to launch a terminal. Supports XDG field codes: "
       "%d = directory of current document."                                                  },

      {"unchange_document",
       setting_value_t{false},
       "Mark new, unsaved, empty documents as unchanged."                                     },

      {"preview_theme",
       setting_value_t{std::string{"system"}},
       "Theme to use for the preview: 'light', 'dark', or 'system'."                          },

      {"custom_css_path",
       setting_value_t{std::string{""}},
       "Optional path to a custom CSS file for styling the preview output."                   }
  };

  std::unordered_map<std::string, setting_value_t> settings_;
  std::unordered_map<std::string, std::string> help_texts_;

  explicit PreviewConfig(const std::filesystem::path &full_path);

  bool load();
  bool save() const;

  // Builds config UI and hooks Apply/OK
  GtkWidget *buildConfigWidget(GtkDialog *dialog);

  template <typename T>
  T get(const std::string &key, T default_val = {}) const {
    if (auto it = settings_.find(key); it != settings_.end()) {
      if (auto val = std::get_if<T>(&it->second)) {
        return *val;
      }
    }
    return default_val;
  }

  template <typename T>
  void set(const std::string &key, T value) {
    settings_[key] = std::move(value);
  }

  std::string getHelp(const std::string &key) const {
    if (auto it = help_texts_.find(key); it != help_texts_.end()) {
      return it->second;
    }
    return {};
  }

 private:
  void onDialogResponse(GtkDialog *dialog, gint response_id);

  std::string config_path_;
};
