// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "util/file_utils.h"  // for readFileToString

namespace {
// Embedded patcher JS (actual code, not a preprocessor directive)
constexpr const char *kApplyPatchJS = R"JS(
// Minimal DOM patcher: replaces only changed children of #root
function parseBody(html) {
  const doc = new DOMParser().parseFromString(html, 'text/html');
  return doc.body || document.createElement('body');
}

function applyPatch(newHtml) {
  const root = document.getElementById('root');
  if (!root) return;

  const nextBody = parseBody(newHtml);
  const oldChildren = Array.from(root.children);
  const newChildren = Array.from(nextBody.children);
  const len = Math.max(oldChildren.length, newChildren.length);

  for (let i = 0; i < len; i++) {
    const oldNode = oldChildren[i];
    const newNode = newChildren[i];

    if (!oldNode && newNode) {
      // Append new node
      root.appendChild(newNode);
    } else if (oldNode && !newNode) {
      // Remove extra old node
      root.removeChild(oldNode);
    } else if (oldNode && newNode) {
      // Replace if different
      if (oldNode.outerHTML !== newNode.outerHTML) {
        root.replaceChild(newNode, oldNode);
      }
    }
  }
}
)JS";
}  // namespace

class WebView final {
 public:
  WebView() noexcept : webview_settings_(webkit_settings_new()) {
    // Allow file:// to load file:// resources
    webkit_settings_set_allow_file_access_from_file_urls(webview_settings_, true);

    // Allow file:// to load remote resources
    webkit_settings_set_allow_universal_access_from_file_urls(webview_settings_, true);

    webview_ = webkit_web_view_new_with_settings(webview_settings_);
    webview_context_ = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview_));
    webview_content_manager_ =
        webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_));

    // Hook load-changed to inject patcher after the new page finishes loading
    g_signal_connect(
        webview_,
        "load-changed",
        G_CALLBACK(+[](WebKitWebView *view, WebKitLoadEvent load_event, gpointer user_data) {
          if (load_event == WEBKIT_LOAD_FINISHED) {
            static_cast<WebView *>(user_data)->injectPatcher();
          }
        }),
        this
    );
  }

  ~WebView() noexcept {
    if (G_IS_OBJECT(webview_settings_)) {
      g_object_unref(webview_settings_);
    }
  }

  GtkWidget *widget() const {
    return webview_;
  }

  WebView &injectPatcher() {
    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_),
        kApplyPatchJS,
        -1,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    return *this;
  }

  WebView &loadHtml(
      std::string_view body_content,
      const std::string &base_uri = "",
      double *scroll_fraction_ptr = nullptr
  ) {
    double fraction = scroll_fraction_ptr ? *scroll_fraction_ptr : internal_scroll_fraction_;
    fraction = std::clamp(fraction, 0.0, 1.0);

    injectBaseUri(base_uri);

    std::string html =
        "<!DOCTYPE html><html><head>"
        "<base href=\"" +
        escapeForJsTemplateLiteral(base_uri) +
        "\">"
        "<meta charset=\"UTF-8\">"
        "<title>Preview</title>"
        "<style></style>"
        "<script>" +
        std::string(kApplyPatchJS) +
        "</script>"
        "</head><body><div id=\"root\">" +
        std::string(body_content) + "</div></body></html>";

    // Actually load the HTML into the WebView
    GBytes *bytes = g_bytes_new_static(html.data(), html.size());
    webkit_web_view_load_bytes(
        WEBKIT_WEB_VIEW(webview_), bytes, "text/html", "UTF-8", "file:///example/example.html"
    );
    g_bytes_unref(bytes);

    // Hook load-changed to inject patcher after the new page finishes loading
    g_signal_connect(
        webview_,
        "load-changed",
        G_CALLBACK(+[](WebKitWebView *view, WebKitLoadEvent load_event, gpointer user_data) {
          if (load_event == WEBKIT_LOAD_FINISHED) {
            static_cast<WebView *>(user_data)->injectPatcher();
          }
        }),
        this
    );

    // Restore scroll after reload
    setScrollFraction(fraction);

    return *this;
  }

  WebView &updateHtml(
      std::string_view body_content,
      const std::string &base_uri = "",
      double *scroll_fraction_ptr = nullptr,
      bool allow_fallback = false
  ) {
    double fraction = scroll_fraction_ptr ? *scroll_fraction_ptr : internal_scroll_fraction_;
    fraction = std::clamp(fraction, 0.0, 1.0);

    std::string escaped = escapeForJsTemplateLiteral(body_content);

    std::string js;
    if (allow_fallback) {
      js = "if (typeof applyPatch === 'function') {"
           "  applyPatch(`" +
           escaped +
           "`);"
           "} else {"
           "  var root = document.getElementById('root');"
           "  if (root) { root.innerHTML = `" +
           escaped +
           "`; }"
           "  else { document.body.innerHTML = `" +
           escaped +
           "`; }"
           "}"
           "window.scrollTo(0, document.body.scrollHeight * " +
           std::to_string(fraction) + ");";
    } else {
      js = "applyPatch(`" + escaped +
           "`);"
           "window.scrollTo(0, document.body.scrollHeight * " +
           std::to_string(fraction) + ");";
    }

    injectBaseUri(base_uri);

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
    );

    return *this;
  }

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
          if (G_IS_OBJECT(val)) {
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

  WebView &setScrollFraction(double fraction) {
    fraction = std::clamp(fraction, 0.0, 1.0);
    std::string js =
        "window.scrollTo(0, document.body.scrollHeight * " + std::to_string(fraction) + ");";
    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
    );
    return *this;
  }

  static std::string escapeForJsTemplateLiteral(std::string_view input) {
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
      unsigned char c = input[i];
      if (c == '`') {
        out += "\\`";
      } else if (c == '\\') {
        out += "\\\\";
      } else if (c == '$' && i + 1 < input.size() && input[i + 1] == '{') {
        out += "\\${";
        ++i;
      } else if (c == '\n') {
        out += "\\n";
      } else if (c == '\r') {
        out += "\\r";
      } else if (c == 0xE2 && i + 2 < input.size() &&
                 static_cast<unsigned char>(input[i + 1]) == 0x80 &&
                 (static_cast<unsigned char>(input[i + 2]) == 0xA8 ||   // U+2028
                  static_cast<unsigned char>(input[i + 2]) == 0xA9)) {  // U+2029
        out += (input[i + 2] == char(0xA8)) ? "\\u2028" : "\\u2029";
        i += 2;
      } else {
        out += c;
      }
    }
    return out;
  }

  WebView &injectBaseUri(const std::string &base_uri) {
    if (base_uri.empty()) {
      return *this;
    }

    std::string escaped_uri = escapeForJsTemplateLiteral(base_uri);
    std::string js =
        "var baseEl = document.querySelector('base') || document.createElement('base');"
        "baseEl.href = '" +
        escaped_uri +
        "';"
        "if (document.head && !baseEl.parentNode) { document.head.prepend(baseEl); };";

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
    );
    return *this;
  }

  WebView &clearInjectedCss() {
    webkit_user_content_manager_remove_all_style_sheets(webview_content_manager_);
    return *this;
  }

  WebView &injectCssFromString(const std::string &css) {
    WebKitUserStyleSheet *sheet = webkit_user_style_sheet_new(
        css.c_str(),
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_STYLE_LEVEL_USER,
        nullptr,
        nullptr
    );
    webkit_user_content_manager_add_style_sheet(webview_content_manager_, sheet);
    return *this;
  }

  WebView &injectCssFromFile(const std::filesystem::path &file) {
    try {
      auto css = FileUtils::readFileToString(file);
      injectCssFromString(css);
    } catch (...) {
      // Silently ignore any failure (missing file, read error, etc.)
    }
    return *this;
  }

  WebView &print(GtkWindow *parent = nullptr) {
    WebKitPrintOperation *op = webkit_print_operation_new(WEBKIT_WEB_VIEW(webview_));
    if (parent) {
      webkit_print_operation_run_dialog(op, parent);
    } else {
      webkit_print_operation_print(op);
    }
    g_object_unref(op);
    return *this;
  }

 private:
  WebKitSettings *webview_settings_ = nullptr;
  GtkWidget *webview_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;

  mutable double internal_scroll_fraction_ = 0.0;
};
