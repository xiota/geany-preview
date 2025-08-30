// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "gtk_attachable.h"

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

  WebView &loadHtml(const std::string &html, const std::string &baseUri = "") {
    webkit_web_view_load_html(
        WEBKIT_WEB_VIEW(webview_), html.c_str(), baseUri.empty() ? nullptr : baseUri.c_str()
    );
    return *this;
  }

 private:
  GtkWidget *webview_ = nullptr;
  WebKitSettings *webviewSettings_ = nullptr;
  WebKitWebContext *webviewContext_ = nullptr;
  WebKitUserContentManager *webviewContentManager_ = nullptr;
};
