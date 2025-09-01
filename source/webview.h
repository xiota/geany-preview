// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

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
    if (scroll_restore_handler_id_ > 0) {
      g_signal_handler_disconnect(webview_, scroll_restore_handler_id_);
    }
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

  WebView &loadPlainText(std::string_view content) {
    return loadMimeType(content, "text/plain", "UTF-8", "");
  }

  WebView &loadHtml(
      std::string_view content,
      const std::string &base_uri = "",
      double *scroll_fraction_ptr = nullptr
  ) {
    return loadMimeType(content, "text/html", "UTF-8", base_uri, scroll_fraction_ptr);
  }

  WebView &loadMimeType(
      std::string_view content,
      const std::string &mime_type = "text/html",
      const std::string &encoding = "UTF-8",
      const std::string &base_uri = "",
      double *scroll_fraction_ptr = nullptr
  ) {
    double *target = scroll_fraction_ptr ? scroll_fraction_ptr : &internal_scroll_fraction_;
    captureScrollFraction(target);

    GBytes *bytes = g_bytes_new_static(content.data(), content.size());
    webkit_web_view_load_bytes(
        WEBKIT_WEB_VIEW(webview_),
        bytes,
        mime_type.empty() ? nullptr : mime_type.c_str(),
        encoding.empty() ? nullptr : encoding.c_str(),
        base_uri.empty() ? nullptr : base_uri.c_str()
    );
    g_bytes_unref(bytes);

    restoreScrollOnLoad(target);
    return *this;
  }

 private:
  void captureScrollFraction(double *target) const {
    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_),
        "document.scrollingElement.scrollTop / document.scrollingElement.scrollHeight",
        -1,
        nullptr,
        nullptr,
        nullptr,
        WebView::onScrollFractionCaptured,
        target
    );
  }

  void restoreScrollOnLoad(double *target) {
    if (scroll_restore_handler_id_ > 0) {
      g_signal_handler_disconnect(webview_, scroll_restore_handler_id_);
      scroll_restore_handler_id_ = 0;
    }

    scroll_restore_handler_id_ = g_signal_connect(
        webview_,
        "load-changed",
        G_CALLBACK(+[](WebKitWebView *wv, WebKitLoadEvent event, gpointer user_data) {
          if (event != WEBKIT_LOAD_FINISHED) {
            return;
          }
          auto *ptr = static_cast<double *>(user_data);
          double fraction = std::clamp(*ptr, 0.0, 1.0);
          std::string js = "window.scrollTo(0, document.scrollingElement.scrollHeight * " +
                           std::to_string(fraction) + ");";
          webkit_web_view_evaluate_javascript(
              wv, js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
          );
        }),
        target
    );
  }

  static void onScrollFractionCaptured(GObject *source, GAsyncResult *res, gpointer user_data) {
    auto *ptr = static_cast<double *>(user_data);
    GError *err = nullptr;
    JSCValue *val =
        webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), res, &err);
    if (!err && jsc_value_is_number(val)) {
      *ptr = jsc_value_to_double(val);
    }
    if (val) {
      g_object_unref(val);
    }
    if (err) {
      g_error_free(err);
    }
  }

  WebKitSettings *webview_settings_ = nullptr;
  GtkWidget *webview_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;

  mutable double internal_scroll_fraction_ = 0.0;
  gulong scroll_restore_handler_id_ = 0;
};
