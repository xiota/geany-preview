// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

class WebView final {
 public:
  WebView() noexcept
      : webview_settings_(webkit_settings_new()),
        webview_(webkit_web_view_new_with_settings(webview_settings_)),
        webview_context_(webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview_))),
        webview_content_manager_(
            webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_))
        ) {}

  ~WebView() noexcept {
    if (webview_settings_) {
      g_object_unref(webview_settings_);
    }
    if (webview_) {
      g_object_unref(webview_);
    }
  }

  GtkWidget *widget() const {
    return webview_;
  }

  WebView &loadUri(const std::string &uri) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview_), uri.c_str());
    return *this;
  }

  WebView &loadMimeType(
      std::string_view content,
      const std::string &mime_type = "text/html",
      const std::string &encoding = "UTF-8",
      const std::string &base_uri = ""
  ) {
    GBytes *bytes = g_bytes_new_static(content.data(), content.size());
    webkit_web_view_load_bytes(
        WEBKIT_WEB_VIEW(webview_),
        bytes,
        mime_type.empty() ? nullptr : mime_type.c_str(),
        encoding.empty() ? nullptr : encoding.c_str(),
        base_uri.empty() ? nullptr : base_uri.c_str()
    );
    g_bytes_unref(bytes);
    return *this;
  }

  WebView &loadPlainText(std::string_view content) {
    return loadMimeType(content, "text/plain", "UTF-8", "");
  }

  WebView &loadHtml(std::string_view content, const std::string &base_uri = "") {
    return loadMimeType(content, "text/html", "UTF-8", base_uri);
  }

 private:
  WebKitSettings *webview_settings_ = nullptr;
  GtkWidget *webview_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;
};
