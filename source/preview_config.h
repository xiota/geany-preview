// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <gtk/gtk.h>

class PreviewConfig {
 public:
  // Expanded to support vectors of ints and strings
  using setting_value_type =
      std::variant<int, bool, std::string, std::vector<int>, std::vector<std::string> >;

  struct SettingDef {
    const char *key;
    setting_value_type default_value;
    const char *help;
  };

  // clang-format off
  inline static const SettingDef setting_defs_[] = {
    { "auto_set_pwd",
      setting_value_type{ false },
      "Keep the working directory (and $PWD) set to the current document's folder. "
      "If it does not exist, walk up until a valid directory is found." },

    { "auto_set_read_only",
      setting_value_type{ false },
      "Automatically set documents to read-only mode when they are not writable." },

    { "color_tooltip",
      setting_value_type{ false },
      "Show hex colors in tooltips when the mouse hovers over them." },

    { "color_tooltip_size",
      setting_value_type{ "small" },
      "Tooltip size: small, medium, or large. The first letter is significant." },

    { "color_chooser",
      setting_value_type{ false },
      "Open the color chooser when double-clicking a color value." },

    { "column_markers",
      setting_value_type{ false },
      "Enable or disable visual column markers in the editor." },

    { "column_markers_columns",
      setting_value_type{ std::vector<int>{ 60, 72, 80, 88, 96, 104, 112, 120, 128 } },
      "List of column positions (in characters) where vertical guide lines will be drawn." },

    { "column_markers_colors",
      setting_value_type{ std::vector<std::string>{ "#ccc", "#bdf", "#fcf", "#ccc", "#fba", "#ccc", "#ccc", "#ccc", "#ccc" } },
      "Colors for each column marker, in #RRGGBB or #RGB format, matching the order of 'column_markers_columns'." },

    { "disable_ctrl_wheel_zoom",
      setting_value_type{ false },
      "Disable zooming the editor with Ctrl+MouseWheel. Keyboard zoom shortcuts remain available." },

    { "file_manager_command",
      setting_value_type{ std::string{ "xdg-open %d" } },
      "Command to launch a file manager. Supports XDG field codes: %d = directory of current document." },

    { "focus_editor_on_raise",
      setting_value_type{ false },
      "Focus the editor when the Geany window is raised." },

    { "headers_incomplete_max",
      setting_value_type{ 3 },
      "Max number of incomplete header lines before treating all as body." },

    { "mark_word",
      setting_value_type{ false },
      "Mark all occurrences of a word when double-clicking it." },

    { "mark_word_double_click_delay",
      setting_value_type{ 50 },
      "Delay in milliseconds before marking all occurrences after a double-click." },

    { "mark_word_single_click_deselect",
      setting_value_type{ true },
      "Deselect the previous highlight by single click." },

    { "redetect_filetype_on_reload",
      setting_value_type{ false },
      "Re-detect file type when a document is reloaded. Geany normally auto-detects file type only on open and save." },

    { "sidebar_auto_resize",
      setting_value_type{ false },
      "Enable or disable automatic resizing of the sidebar based on window state "
      "(normal, maximized, fullscreen)." },

    { "sidebar_resize_delay",
      setting_value_type{ 50 },
      "Delay in milliseconds before applying the automatic sidebar resize after a window "
      "state or size change. A small delay helps avoid race conditions with GTK/Geany's "
      "own layout adjustments. Set to 0 to apply immediately." },

    { "sidebar_columns_fullscreen",
      setting_value_type{ 94 },
      "Number of editor columns to keep visible when the window is in fullscreen mode. "
      "The sidebar width will be adjusted accordingly." },

    { "sidebar_columns_maximized",
      setting_value_type{ 94 },
      "Number of editor columns to keep visible when the window is maximized. "
      "The sidebar width will be adjusted accordingly." },

    { "sidebar_columns_normal",
      setting_value_type{ 82 },
      "Number of editor columns to keep visible when the window is in normal (non-maximized) state. "
      "The sidebar width will be adjusted accordingly." },

    { "sidebar_size_min",
      setting_value_type{ 150 },
      "Minimum width of the sidebar in pixels. "
      "Prevents automatic resizing from shrinking the sidebar too far." },

    { "terminal_command",
      setting_value_type{ std::string{ "xdg-terminal-exec --working-directory=%d" } },
      "Command to launch a terminal. Supports XDG field codes: %d = directory of current document." },

    { "theme_mode",
      setting_value_type{ std::string{ "system" } },
      "Theme to use for the preview: 'light', 'dark', or 'system'." },

    { "unchange_document",
      setting_value_type{ false },
      "Mark new, unsaved, empty documents as unchanged." },

    { "update_cooldown",
      setting_value_type{ 65 },
      "Time in milliseconds to wait between preview updates to prevent excessive refreshing." },

    { "update_min_delay",
      setting_value_type{ 15 },
      "Time in milliseconds to wait before the first preview update after a change." },
  };
  // clang-format on

  std::unordered_map<std::string, setting_value_type> settings_;
  std::unordered_map<std::string, std::string> help_texts_;

  explicit PreviewConfig(
      const std::filesystem::path &config_path,
      std::string_view config_file
  );

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

  const std::filesystem::path &configDir() const noexcept {
    return config_path_;
  }

 private:
  void onDialogResponse(GtkDialog *dialog, gint response_id);
  GtkListStore *createConfigModel();
  GtkTreeView *createConfigTreeView(GtkListStore *store);
  void connectConfigSignals(GtkTreeView *tree_view, GtkDialog *dialog);

  std::filesystem::path config_path_;
  std::string config_file_;
};
