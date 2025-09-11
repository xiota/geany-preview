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

    g_signal_connect_after(webview_, "context-menu", G_CALLBACK(onContextMenu), this);
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

  WebView &findText(
      const std::string &text,
      WebKitFindOptions options = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE,
      guint maxMatches = G_MAXUINT
  ) {
    auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
    webkit_find_controller_search(fc, text.c_str(), options, maxMatches);
    return *this;
  }

  WebView &findNext() {
    auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
    webkit_find_controller_search_next(fc);
    return *this;
  }

  WebView &findPrevious() {
    auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
    webkit_find_controller_search_previous(fc);
    return *this;
  }

  WebView &clearFind() {
    auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
    webkit_find_controller_search_finish(fc);
    return *this;
  }

 private:
  static void showFindPrompt(WebView *wv, GtkWindow *parent_window) {
    // Track the currently open prompt so we don't spawn duplicates
    static GtkWidget *active_find_window = nullptr;

    if (active_find_window && GTK_IS_WINDOW(active_find_window)) {
      gtk_window_present(GTK_WINDOW(active_find_window));
      return;
    }

    GtkWidget *find_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    active_find_window = find_window;

    gtk_window_set_title(GTK_WINDOW(find_window), "Find in Page");
    gtk_window_set_transient_for(GTK_WINDOW(find_window), parent_window);
    gtk_window_set_modal(GTK_WINDOW(find_window), false);
    gtk_container_set_border_width(GTK_CONTAINER(find_window), 6);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    GtkWidget *entry_search = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_search), "Search text…");
    gtk_entry_set_activates_default(GTK_ENTRY(entry_search), true);

    GtkWidget *btn_next = gtk_button_new_with_label("Next");
    GtkWidget *btn_prev = gtk_button_new_with_label("Previous");

    gtk_box_pack_start(GTK_BOX(hbox), entry_search, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn_next, false, false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn_prev, false, false, 0);

    gtk_container_add(GTK_CONTAINER(find_window), hbox);

    // Live search
    g_signal_connect(
        entry_search,
        "changed",
        G_CALLBACK(+[](GtkEntry *entry, gpointer data) {
          auto *wv = static_cast<WebView *>(data);
          const char *text = gtk_entry_get_text(entry);
          if (text && *text) {
            wv->findText(text);
          } else {
            wv->clearFind();
          }
        }),
        wv
    );

    // Enter triggers "Next"
    g_signal_connect(
        entry_search,
        "activate",
        G_CALLBACK(+[](GtkEntry *, gpointer data) {
          static_cast<WebView *>(data)->findNext();
        }),
        wv
    );

    // Navigation buttons
    g_signal_connect(
        btn_next,
        "clicked",
        G_CALLBACK(+[](GtkButton *, gpointer data) {
          static_cast<WebView *>(data)->findNext();
        }),
        wv
    );

    g_signal_connect(
        btn_prev,
        "clicked",
        G_CALLBACK(+[](GtkButton *, gpointer data) {
          static_cast<WebView *>(data)->findPrevious();
        }),
        wv
    );

    // Close on Esc
    g_signal_connect(
        find_window,
        "key-press-event",
        G_CALLBACK(+[](GtkWidget *widget, GdkEventKey *event, gpointer) -> gboolean {
          if (event->keyval == GDK_KEY_Escape) {
            gtk_widget_destroy(widget);
            return true;
          }
          return false;
        }),
        nullptr
    );

    // Clear highlights and reset static pointer when closed
    g_signal_connect(
        find_window,
        "destroy",
        G_CALLBACK(+[](GtkWidget *, gpointer data) {
          static_cast<WebView *>(data)->clearFind();
          active_find_window = nullptr;
        }),
        wv
    );

    gtk_widget_show_all(find_window);

    // --- Center over the WebView in the sidebar ---
    if (gtk_widget_get_window(wv->widget())) {
      int wx = 0, wy = 0;
      gdk_window_get_origin(gtk_widget_get_window(wv->widget()), &wx, &wy);

      int w_width = gtk_widget_get_allocated_width(wv->widget());
      int w_height = gtk_widget_get_allocated_height(wv->widget());

      GtkRequisition req;
      gtk_widget_get_preferred_size(find_window, &req, nullptr);
      int f_width = req.width;
      int f_height = req.height;

      int pos_x = wx + (w_width - f_width) / 2;
      int pos_y = wy + (w_height - f_height) / 2;

      gtk_window_move(GTK_WINDOW(find_window), pos_x, pos_y);
    }
  }

  static gboolean onContextMenu(
      WebKitWebView *view,
      WebKitContextMenu *menu,
      GdkEvent *event,
      WebKitHitTestResult *hit_test_result,
      gpointer user_data
  ) {
    auto *self = static_cast<WebView *>(user_data);

    // Prune unwanted defaults
    if (GList *items = webkit_context_menu_get_items(menu)) {
      for (GList *l = items; l != nullptr;) {
        WebKitContextMenuItem *menu_item = static_cast<WebKitContextMenuItem *>(l->data);
        WebKitContextMenuAction action = webkit_context_menu_item_get_stock_action(menu_item);
        l = l->next;
        if (action == WEBKIT_CONTEXT_MENU_ACTION_GO_BACK ||
            action == WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD ||
            action == WEBKIT_CONTEXT_MENU_ACTION_STOP) {
          webkit_context_menu_remove(menu, menu_item);
        }
      }
    }

    // Separator
    webkit_context_menu_append(menu, webkit_context_menu_item_new_separator());

    // GAction-backed "Find in Page…" item
    GSimpleAction *find_action = g_simple_action_new("find_in_page", nullptr);
    g_signal_connect(
        find_action,
        "activate",
        G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
          auto *wv = static_cast<WebView *>(data);
          GtkWidget *toplevel = gtk_widget_get_toplevel(wv->widget());
          if (GTK_IS_WINDOW(toplevel)) {
            showFindPrompt(wv, GTK_WINDOW(toplevel));
          } else {
            showFindPrompt(wv, nullptr);
          }
        }),
        self
    );

    WebKitContextMenuItem *find_item = webkit_context_menu_item_new_from_gaction(
        G_ACTION(find_action), "Find in Page…", nullptr
    );
    webkit_context_menu_append(menu, find_item);

    g_object_unref(find_action);
    return false;
  }

  WebKitSettings *webview_settings_ = nullptr;
  GtkWidget *webview_ = nullptr;
  WebKitWebContext *webview_context_ = nullptr;
  WebKitUserContentManager *webview_content_manager_ = nullptr;

  mutable double internal_scroll_fraction_ = 0.0;
};
