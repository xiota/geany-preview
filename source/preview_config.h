// SPDX-FileCopyrightText: Copyright 2025-2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <gtk/gtk.h>

class PreviewConfig {
 public:
  static PreviewConfig &init(const std::filesystem::path &path, std::string_view file) {
    static bool initialized = false;
    auto &cfg = instance();

    if (!initialized) {
      cfg.setConfigPath(path);
      cfg.setConfigFile(file);
      initialized = true;
    }

    return cfg;
  }

  static PreviewConfig &instance() {
    static PreviewConfig inst;
    return inst;
  }

 private:
  PreviewConfig();
  ~PreviewConfig() = default;

  PreviewConfig(const PreviewConfig &) = delete;
  PreviewConfig &operator=(const PreviewConfig &) = delete;
  PreviewConfig(PreviewConfig &&) = delete;
  PreviewConfig &operator=(PreviewConfig &&) = delete;

 public:
  using setting_value_type =
      std::variant<int, double, bool, std::string, std::vector<int>, std::vector<std::string> >;

 private:
  struct SettingDef {
    const char *key;
    setting_value_type default_value;
    const char *help;
  };

  // clang-format off
  inline static std::vector<SettingDef> setting_defs_ = {
    { "disable_preview_ctrl_wheel_zoom",
      setting_value_type{ false },
      "Disable Ctrl+MouseWheel zoom in the preview pane." },

    { "file_manager_command",
      setting_value_type{ std::string{ "xdg-open %d" } },
      "Command to launch a file manager. %d = current document directory." },

    { "headers_incomplete_max",
      setting_value_type{ 3 },
      "Max number of incomplete header lines before treating all as body." },

    { "keybinding_behavior_strict",
      setting_value_type{ false },
      "Change focus only if editor or preview/sidebar has focus; otherwise do nothing." },

    { "preview_base_path",
      setting_value_type{ std::string{ "sandbox" } },
      "Base path for preview pane resources. "
      "Affects interpretation of relative links. "
      "Supports variable expansion ($PWD, $HOME, $XDG_DOCUMENTS_DIR)." },

    { "preview_show_extra_info",
      setting_value_type{ false },
      "Show extra information when the document cannot be rendered." },

    { "preview_zoom_default",
      setting_value_type{ 1.0 },
      "Default zoom level for the preview pane.  (1.0 = 100%)" },

    { "preview_zoom_sync",
      setting_value_type{ false },
      "Synchronize zoom changes between Preview and Editor" },

    { "terminal_command",
      setting_value_type{ std::string{ "xdg-terminal-exec --working-directory=%d" } },
      "Command to launch a terminal. %d = current document directory." },

    { "theme_mode",
      setting_value_type{ std::string{ "system" } },
      "Theme to use for the preview: 'light', 'dark', or 'system'." },

    { "update_cooldown",
      setting_value_type{ 65 },
      "Cooldown (ms) between preview updates to avoid excessive refresh." },

    { "update_min_delay",
      setting_value_type{ 15 },
      "Delay (ms) before first preview update after a change." },

    { "webview_resize_buffer",
      setting_value_type{ 0 },
      "Extra pixels to add when expanding the preview pane to avoid flicker." },
  };
  // clang-format on

 public:
  static void
  registerSetting(const char *key, setting_value_type default_value, const char *help) {
    // Extend the master table so the GUI sees it
    setting_defs_.push_back({ key, default_value, help });

    // Insert into runtime maps
    auto &cfg = instance();
    cfg.settings_[key] = default_value;
    cfg.help_texts_[key] = help;
  }

  std::unordered_map<std::string, setting_value_type> settings_;
  std::unordered_map<std::string, std::string> help_texts_;

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
  void set(const std::string &key, const T &value) {
    settings_[key] = value;
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

  // for change signal
  using Callback = std::function<void()>;

  void connectChanged(Callback cb) {
    listeners_.push_back(std::move(cb));
  }

 private:
  void setConfigPath(const std::filesystem::path &p) {
    config_path_ = std::filesystem::weakly_canonical(p);
    std::filesystem::create_directories(config_path_);
  }

  void setConfigFile(std::string_view f) {
    config_file_ = f;
  }

 private:
  // for change signal
  std::vector<Callback> listeners_;

  void emitChanged() {
    for (auto &cb : listeners_) {
      cb();
    }
  }

  void onDialogResponse(GtkDialog *dialog, gint response_id);
  GtkListStore *createConfigModel();
  GtkTreeView *createConfigTreeView(GtkListStore *store);
  void connectConfigSignals(GtkTreeView *tree_view, GtkDialog *dialog);

  std::filesystem::path config_path_;
  std::string config_file_;
};
