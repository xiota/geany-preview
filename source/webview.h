// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "gtk_attachable.h"

class WebView : public GtkAttachable<WebView> {
 public:
  WebView() : GtkAttachable("WebView") {
    webview_settings_ = webkit_settings_new();
    webview_ = webkit_web_view_new_with_settings(webview_settings_);
    webview_context_ = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview_));
    webview_content_manager_ =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_));

    TrackWidget(GTK_WIDGET(webview_settings_));
    TrackWidget(GTK_WIDGET(webview_context_));
    TrackWidget(GTK_WIDGET(webview_content_manager_));
    TrackWidget(GTK_WIDGET(webview_));
  }

  GtkWidget *widget() const override {
    return webview_;
  }

  WebView &LoadUri(const std::string &uri) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview_), uri.c_str());
    return *this;
  }

  WebView &LoadHtml(const std::string &html, const std::string &base_uri = "") {
    webkit_web_view_load_html(
        WEBKIT_WEB_VIEW(webview_), html.c_str(), base_uri.empty() ? nullptr : base_uri.c_str()
    );
    return *this;
  }

 private:
  GtkWidget *webview_ = nullptr;
  WebKitSettings *webview_settings_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;
};
