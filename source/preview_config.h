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

  // clang-format off
  inline static const setting_def_t setting_defs_[] = {
    { "allow_update_fallback",
      setting_value_t{ false},
      "Whether to use fallback rendering mode if DOM patch mode fails." },

    { "auto_set_read_only",
      setting_value_t{ false},
      "Automatically set documents to read-only mode when they are not writable." },

    { "color_tooltip",
      setting_value_t{ false},
      "Show hex colors in tooltips when the mouse hovers over them." },

    { "color_tooltip_size",
      setting_value_t{ "small"},
      "Tooltip size: small, medium, or large. The first letter is significant." },

    { "color_chooser",
      setting_value_t{ false},
      "Open the color chooser when double-clicking a color value." },

    { "column_markers",
      setting_value_t{ false},
      "Enable or disable visual column markers in the editor." },

    { "column_markers_columns",
      setting_value_t{ std::vector<int>{ 60, 72, 80, 88, 96, 104, 112, 120, 128 }},
      "List of column positions (in characters) where vertical guide lines will be drawn." },

    { "column_markers_colors",
      setting_value_t{ std::vector<std::string>{ "#ccc", "#bdf", "#fcf", "#ccc", "#fba", "#ccc", "#ccc", "#ccc", "#ccc" }},
      "Colors for each column marker, in #RRGGBB or #RGB format, matching the order of 'column_markers_columns'." },

    { "mark_word",
      setting_value_t{ false},
      "Mark all occurrences of a word when double-clicking it." },

    { "mark_word_double_click_delay",
      setting_value_t{ 50},
      "Delay in milliseconds before marking all occurrences after a double-click." },

    { "mark_word_single_click_deselect",
      setting_value_t{ true},
      "Deselect the previous highlight by single click." },

    { "redetect_filetype_on_reload",
      setting_value_t{ false},
      "Re-detect file type when a document is reloaded. Geany normally auto-detects file type only on open and save." },

    { "terminal_command",
      setting_value_t{ std::string{ "xdg-terminal-exec --working-directory=%d" }},
      "Command to launch a terminal. Supports XDG field codes: %d = directory of current document." },

    { "unchange_document",
      setting_value_t{ false},
      "Mark new, unsaved, empty documents as unchanged." },

    { "update_cooldown",
      setting_value_t{ 65},
      "Time in milliseconds to wait between preview updates to prevent excessive refreshing." },

    { "update_min_delay",
      setting_value_t{ 15},
      "Time in milliseconds to wait before the first preview update after a change." },

    { "preview_theme",
      setting_value_t{ std::string{ "system" }},
      "Theme to use for the preview: 'light', 'dark', or 'system'." },

    { "custom_css_path",
      setting_value_t{ std::string{ "" }},
      "Optional path to a custom CSS file for styling the preview output." }
  };
  // clang-format on

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
  GtkListStore *createConfigModel();
  GtkTreeView *createConfigTreeView(GtkListStore *store);
  void connectConfigSignals(GtkTreeView *tree_view, GtkDialog *dialog);

  std::string config_path_;
};
