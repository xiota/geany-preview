// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <gtk/gtk.h>

#include "converter_registrar.h"
#include "document.h"
#include "preview_config.h"
#include "preview_context.h"
#include "webview.h"

class PreviewPane final {
 public:
  explicit PreviewPane(PreviewContext *context);
  ~PreviewPane() noexcept;

  GtkWidget *widget() const;

  PreviewPane &initWebView(const Document &document);
  PreviewPane &checkHealth();
  PreviewPane &scheduleUpdate();

  void triggerUpdate(const Document &document);

  void
  exportHtmlToFileAsync(const std::filesystem::path &dest, std::function<void(bool)> callback);

  void
  exportPdfToFileAsync(const std::filesystem::path &dest, std::function<void(bool)> callback);

  bool canPreviewFile(const Document &doc) const;

 private:
  void connectWebViewSignals();
  void safeReparentWebView(GtkWidget *new_parent, bool pack_into_box);
  std::string generateHtml(const Document &document) const;
  std::string calculateBaseUri(const Document &document) const;
  PreviewPane &update(const Document &document);
  void addWatchIfNeeded(const std::filesystem::path &path);
  void stopAllWatches();

  PreviewPane &clearAndReloadCss();
  PreviewPane &injectCssTheme();

  PreviewContext *context_;
  gulong init_handler_id_ = 0;

  GtkWidget *sidebar_notebook_;
  GtkWidget *page_box_ = nullptr;
  GtkWidget *offscreen_ = nullptr;
  guint sidebar_page_number_ = 0;
  gulong sidebar_switch_page_handler_id_ = 0;

  PreviewConfig *preview_config_;

  WebView webview_;
  ConverterRegistrar registrar_;

  bool update_pending_ = false;
  gint64 last_update_time_ = 0;

  std::unordered_map<std::string, double> scroll_by_file_;
  std::string previous_key_ = "markdown";
  std::string previous_theme_ = "system";

  std::unordered_map<std::filesystem::path, FileUtils::FileWatchHandle> watches_;
  std::string previous_base_uri_;

  bool webview_healthy_ = false;
  std::string root_id_;
};
