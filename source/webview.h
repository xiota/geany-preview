// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "gtk_attachable.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

class WebView : public GtkAttachable<WebView> {
 public:
  WebView() : GtkAttachable("WebView") {
    webviewSettings_ = webkit_settings_new();
    webview_ = webkit_web_view_new_with_settings(webviewSettings_);
    webviewContext_ = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview_));
    webviewContentManager_ =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_));
  }

  GtkWidget *widget() const override {
    return webview_;
  }

  WebView &loadUri(const std::string &uri) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview_), uri.c_str());
    return *this;
  }

  WebView &loadHtml(std::string_view html, const std::string &base_uri = "") {
    GBytes *bytes = g_bytes_new_static(html.data(), html.size());
    webkit_web_view_load_bytes(
        WEBKIT_WEB_VIEW(webview_),
        bytes,
        "text/html",
        "UTF-8",
        base_uri.empty() ? nullptr : base_uri.c_str()
    );
    g_bytes_unref(bytes);
    return *this;
  }

 private:
  GtkWidget *webview_ = nullptr;
  WebKitSettings *webviewSettings_ = nullptr;
  WebKitWebContext *webviewContext_ = nullptr;
  WebKitUserContentManager *webviewContentManager_ = nullptr;
};
