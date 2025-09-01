// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

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

  // --- Full reload with minimal shell ---
  WebView &loadHtml(
      std::string_view body_content,
      const std::string &base_uri = "",
      double *scroll_fraction_ptr = nullptr
  ) {
    double fraction = scroll_fraction_ptr ? *scroll_fraction_ptr : internal_scroll_fraction_;
    fraction = std::clamp(fraction, 0.0, 1.0);

    std::string html =
        "<!DOCTYPE html><html lang=\"en\"><head>"
        "<meta charset=\"UTF-8\">"
        "<title>Preview</title>"
        "<style>"
        "body { margin:0; padding:1rem; font-family:sans-serif; background:#fff; color:#000; }"
        "</style>"
        "</head><body>" +
        std::string(body_content) + "</body></html>";

    GBytes *bytes = g_bytes_new_static(html.data(), html.size());
    webkit_web_view_load_bytes(
        WEBKIT_WEB_VIEW(webview_),
        bytes,
        "text/html",
        "UTF-8",
        base_uri.empty() ? nullptr : base_uri.c_str()
    );
    g_bytes_unref(bytes);

    // Restore scroll after reload
    setScrollFraction(fraction);

    return *this;
  }

  // --- In-place body swap ---
  WebView &updateHtml(std::string_view body_content, double *scroll_fraction_ptr = nullptr) {
    double fraction = scroll_fraction_ptr ? *scroll_fraction_ptr : internal_scroll_fraction_;
    fraction = std::clamp(fraction, 0.0, 1.0);

    std::string escaped = escapeForJsTemplateLiteral(body_content);

    std::string js =
        "if (!document.body) {"
        "  document.write('<!DOCTYPE html><html><head><meta "
        "charset=\"UTF-8\"></head><body></body></html>');"
        "  document.close();"
        "}"
        "document.body.innerHTML = `" +
        escaped +
        "`;"
        "window.scrollTo(0, document.body.scrollHeight * " +
        std::to_string(fraction) + ");";

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
    );

    return *this;
  }

  // --- Async getter for scroll fraction ---
  void getScrollFraction(std::function<void(double)> callback) const {
    auto *cb_ptr = new std::function<void(double)>(std::move(callback));

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_),
        "window.scrollY / document.body.scrollHeight",
        -1,
        nullptr,
        nullptr,
        nullptr,
        [](GObject *source, GAsyncResult *res, gpointer user_data) {
          auto *cb = static_cast<std::function<void(double)> *>(user_data);
          double fraction = 0.0;
          GError *err = nullptr;
          JSCValue *val =
              webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), res, &err);
          if (!err && jsc_value_is_number(val)) {
            fraction = jsc_value_to_double(val);
          }
          if (val) {
            g_object_unref(val);
          }
          if (err) {
            g_error_free(err);
          }
          (*cb)(fraction);
          delete cb;
        },
        cb_ptr
    );
  }

  // --- Direct setter for scroll fraction ---
  void setScrollFraction(double fraction) {
    fraction = std::clamp(fraction, 0.0, 1.0);
    std::string js =
        "window.scrollTo(0, document.body.scrollHeight * " + std::to_string(fraction) + ");";
    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
    );
  }

  static std::string escapeForJsTemplateLiteral(std::string_view input) {
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
      char c = input[i];

      if (c == '`') {
        out += "\\`";  // escape backtick
      } else if (c == '\\') {
        out += "\\\\";  // escape backslash
      } else if (c == '$' && i + 1 < input.size() && input[i + 1] == '{') {
        out += "\\${";  // prevent template interpolation
        ++i;            // skip '{'
      } else if (c == '\n') {
        out += "\\n";  // escape newline
      } else if (c == '\r') {
        out += "\\r";  // escape carriage return
      } else {
        out += c;  // normal char
      }
    }

    return out;
  }

 private:
  WebKitSettings *webview_settings_ = nullptr;
  GtkWidget *webview_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;

  mutable double internal_scroll_fraction_ = 0.0;
};
