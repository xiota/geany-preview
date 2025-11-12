// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "preview_context.h"
#include "util/file_utils.h"

class WebView final {
 public:
  explicit WebView(PreviewContext *context) noexcept;
  ~WebView() noexcept;

  GtkWidget *widget() const;

  WebView &injectPatcher();
  WebView &loadHtml(
      std::string_view body_content,
      const std::string &base_uri,
      std::string_view root_id,
      double *scroll_fraction_ptr
  );

  WebView &updateHtml(
      std::string_view body_content,
      const std::string &base_uri,
      std::string_view root_id,
      double *scroll_fraction_ptr
  );
  void getScrollFraction(std::function<void(double)> callback) const;
  WebView &setScrollFraction(double fraction);
  static std::string escapeForJsTemplateLiteral(std::string_view input);
  WebView &injectBaseUri(const std::string &base_uri, std::string_view root_id);
  WebView &clearInjectedCss();
  WebView &injectCssFromLiteral(const char *css);
  WebView &injectCssFromString(const std::string &css);
  WebView &injectCssFromFile(const std::filesystem::path &file);
  WebView &print(GtkWindow *parent = nullptr);
  WebView &findText(
      const std::string &text,
      WebKitFindOptions options = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE,
      guint maxMatches = G_MAXUINT
  );
  WebView &findNext();
  WebView &findPrevious();
  WebView &clearFind();

  WebView &showFindPrompt(GtkWindow *parent_window);

 private:
  static gboolean onContextMenu(
      WebKitWebView *view,
      WebKitContextMenu *menu,
      GdkEvent *event,
      WebKitHitTestResult *hit_test_result,
      gpointer user_data
  );

  static void onDecidePolicy(
      WebKitWebView *view,
      WebKitPolicyDecision *decision,
      WebKitPolicyDecisionType type,
      gpointer user_data
  );

  static gboolean onScrollEvent(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
  double zoom_level_ = 1.0;

  PreviewContext *context_;
  WebKitSettings *webview_settings_ = nullptr;
  GtkWidget *webview_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;
  mutable double internal_scroll_fraction_ = 0.0;
};
