// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webview.h"

#include <algorithm>
#include <string>
#include <string_view>

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "document_local.h"
#include "preview_context.h"
#include "preview_pane.h"
#include "util/file_utils.h"
#include "util/string_utils.h"
#include "webview_find_dialog.h"

// Anonymous namespace for internal constants
namespace {

constexpr const char *kApplyPatchJS = R"JS(
// Minimal DOM patcher: replaces only changed children of #root
function parseBody(html) {
  const doc = new DOMParser().parseFromString(html, 'text/html');
  return doc.body || document.createElement('body');
}

function applyPatch(newHtml, root_id) {
  root = document.getElementById(root_id);
  if (!root) return;

  const nextBody = parseBody(newHtml);
  const oldChildren = Array.from(root.children);
  const newChildren = Array.from(nextBody.children);
  const len = Math.max(oldChildren.length, newChildren.length);

  for (let i = 0; i < len; i++) {
    const oldNode = oldChildren[i];
    const newNode = newChildren[i];

    if (!oldNode && newNode) {
      root.appendChild(newNode);
    } else if (oldNode && !newNode) {
      root.removeChild(oldNode);
    } else if (oldNode && newNode) {
      if (oldNode.outerHTML !== newNode.outerHTML) {
        root.replaceChild(newNode, oldNode);
      }
    }
  }
}
)JS";

}  // namespace

WebView::WebView(PreviewContext *context) noexcept : context_(context) {}

void WebView::reset() {
  if (GTK_IS_WIDGET(webview_)) {
    gtk_widget_destroy(webview_);
    webview_ = nullptr;
  }
  if (G_IS_OBJECT(webview_settings_)) {
    g_object_unref(webview_settings_);
    webview_settings_ = nullptr;
  }

  webview_settings_ = webkit_settings_new();
  webkit_settings_set_allow_file_access_from_file_urls(webview_settings_, true);
  webkit_settings_set_allow_universal_access_from_file_urls(webview_settings_, true);

  WebKitWebsiteDataManager *manager =
      webkit_website_data_manager_new("disk-cache-directory", NULL, NULL);

  // custom context with no disk cache directory
  webview_context_ = webkit_web_context_new_with_website_data_manager(manager);

  webview_ = webkit_web_view_new_with_context(webview_context_);
  webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webview_), webview_settings_);

  // minimize caching (in‑memory only, no disk persistence)
  webkit_web_context_set_cache_model(webview_context_, WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);

  webview_content_manager_ =
      webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview_));

  double zoom = 100;
  if (context_ && context_->preview_config_) {
    zoom = context_->preview_config_->get<int>("preview_zoom_default", 100);
  }
  zoom = std::max(zoom / 100.0, 0.01);
  webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webview_), zoom);

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
  g_signal_connect(webview_, "decide-policy", G_CALLBACK(onDecidePolicy), this);
  g_signal_connect(webview_, "scroll-event", G_CALLBACK(onScrollEvent), this);
}

WebView::~WebView() noexcept {
  if (G_IS_OBJECT(webview_settings_)) {
    g_object_unref(webview_settings_);
  }
}

GtkWidget *WebView::widget() const {
  return webview_;
}

WebView &WebView::injectPatcher() {
  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(webview_), kApplyPatchJS, -1, nullptr, nullptr, nullptr, nullptr, nullptr
  );
  return *this;
}

WebView &WebView::loadHtml(
    std::string_view body_content,
    const std::string &base_uri,
    std::string_view root_id,
    double *scroll_fraction_ptr
) {
  double fraction = scroll_fraction_ptr ? *scroll_fraction_ptr : 0.0;
  fraction = std::clamp(fraction, 0.0, 1.0);

  std::string head =
      "<!DOCTYPE html><html><head>"
      "<meta charset=\"UTF-8\">"
      "<title>Preview</title>"
      "<style></style>"
      "<script>" +
      std::string(kApplyPatchJS) + "</script>";

  std::string html = head + "</head><body><div id=\"" + StringUtils::escapeHtml(root_id) +
                     "\">" + std::string(body_content) + "</div></body></html>";

  webkit_web_view_load_html(WEBKIT_WEB_VIEW(webview_), html.c_str(), base_uri.c_str());

  setScrollFraction(fraction);
  return *this;
}

WebView &WebView::updateHtml(
    std::string_view body_content,
    const std::string &base_uri,
    std::string_view root_id,
    double *scroll_fraction_ptr
) {
  double fraction = scroll_fraction_ptr ? *scroll_fraction_ptr : 0.0;
  fraction = std::clamp(fraction, 0.0, 1.0);

  std::string escaped = escapeForJsTemplateLiteral(body_content);

  std::string js;
  js = "applyPatch(`" + escaped + "`, `" + escapeForJsTemplateLiteral(root_id) +
       "`);"
       "window.scrollTo(0, document.body.scrollHeight * " +
       std::to_string(fraction) + ");";

  injectBaseUri(base_uri, root_id);

  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
  );

  return *this;
}

void WebView::getScrollFraction(std::function<void(double)> callback) const {
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

WebView &WebView::setScrollFraction(double fraction) {
  fraction = std::clamp(fraction, 0.0, 1.0);
  std::string js =
      "window.scrollTo(0, document.body.scrollHeight * " + std::to_string(fraction) + ");";
  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(webview_), js.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr
  );
  return *this;
}

std::string WebView::escapeForJsTemplateLiteral(std::string_view input) {
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
               (static_cast<unsigned char>(input[i + 2]) == 0xA8 ||
                static_cast<unsigned char>(input[i + 2]) == 0xA9)) {
      out += (input[i + 2] == char(0xA8)) ? "\\u2028" : "\\u2029";
      i += 2;
    } else {
      out += c;
    }
  }
  return out;
}

WebView &WebView::injectBaseUri(const std::string &base_uri, std::string_view root_id) {
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

WebView &WebView::clearInjectedCss() {
  webkit_user_content_manager_remove_all_style_sheets(webview_content_manager_);
  return *this;
}

WebView &WebView::injectCssFromLiteral(const char *css) {
  WebKitUserStyleSheet *sheet = webkit_user_style_sheet_new(
      css, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_STYLE_LEVEL_USER, nullptr, nullptr
  );
  webkit_user_content_manager_add_style_sheet(webview_content_manager_, sheet);
  return *this;
}

WebView &WebView::injectCssFromString(const std::string &css) {
  return injectCssFromLiteral(css.c_str());
}

WebView &WebView::injectCssFromFile(const std::filesystem::path &file) {
  try {
    auto css = FileUtils::readFileToString(file);
    injectCssFromString(css);
  } catch (...) {
    // Silently ignore any failure
  }
  return *this;
}

WebView &WebView::print(GtkWindow *parent) {
  WebKitPrintOperation *op = webkit_print_operation_new(WEBKIT_WEB_VIEW(webview_));
  if (parent) {
    webkit_print_operation_run_dialog(op, parent);
  } else {
    webkit_print_operation_print(op);
  }
  g_object_unref(op);
  return *this;
}

WebView &
WebView::findText(const std::string &text, WebKitFindOptions options, guint maxMatches) {
  auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
  webkit_find_controller_search(fc, text.c_str(), options, maxMatches);
  return *this;
}

WebView &WebView::findNext() {
  auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
  webkit_find_controller_search_next(fc);
  return *this;
}

WebView &WebView::findPrevious() {
  auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
  webkit_find_controller_search_previous(fc);
  return *this;
}

WebView &WebView::clearFind() {
  auto *fc = webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_));
  webkit_find_controller_search_finish(fc);
  return *this;
}

WebView &WebView::showFindPrompt(GtkWindow *parent) {
  if (!find_dialog_) {
    find_dialog_ = std::make_unique<WebViewFindDialog>(this, context_);
  }
  find_dialog_->show(parent);
  return *this;
}

gboolean WebView::onContextMenu(
    WebKitWebView *view,
    WebKitContextMenu *menu,
    GdkEvent *,
    WebKitHitTestResult *,
    gpointer user_data
) {
  auto *self = static_cast<WebView *>(user_data);

  if (GList *items = webkit_context_menu_get_items(menu)) {
    for (GList *l = items; l != nullptr;) {
      WebKitContextMenuItem *menu_item = static_cast<WebKitContextMenuItem *>(l->data);
      WebKitContextMenuAction action = webkit_context_menu_item_get_stock_action(menu_item);
      l = l->next;
      if (action == WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_GO_BACK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD ||
          action == WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW ||
          action == WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW ||
          action == WEBKIT_CONTEXT_MENU_ACTION_RELOAD ||
          action == WEBKIT_CONTEXT_MENU_ACTION_STOP) {
        webkit_context_menu_remove(menu, menu_item);
      }
    }
  }

  webkit_context_menu_append(menu, webkit_context_menu_item_new_separator());

  GSimpleAction *find_action = g_simple_action_new("find_in_page", nullptr);
  g_signal_connect(
      find_action,
      "activate",
      G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
        auto *wv = static_cast<WebView *>(data);
        GtkWidget *toplevel = gtk_widget_get_toplevel(wv->widget());
        if (GTK_IS_WINDOW(toplevel)) {
          wv->showFindPrompt(GTK_WINDOW(toplevel));
        } else {
          wv->showFindPrompt(nullptr);
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

void WebView::onDecidePolicy(
    WebKitWebView *view,
    WebKitPolicyDecision *decision,
    WebKitPolicyDecisionType type,
    gpointer user_data
) {
  WebView *self = static_cast<WebView *>(user_data);

  switch (type) {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION: {
      auto *nav = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
      WebKitNavigationAction *action =
          webkit_navigation_policy_decision_get_navigation_action(nav);

      WebKitURIRequest *req = webkit_navigation_action_get_request(action);
      const gchar *uri = webkit_uri_request_get_uri(req);

      if (!uri) {
        webkit_policy_decision_use(decision);
        return;
      }

      const gchar *scheme = g_uri_parse_scheme(uri);
      if (scheme && g_str_equal(scheme, "file")) {
        gchar *filename = g_filename_from_uri(uri, nullptr, nullptr);
        if (filename) {
          DocumentLocal doc(filename);

          if (self->context_ && self->context_->preview_pane_) {
            if (self->context_->preview_pane_->canPreviewFile(doc)) {
              self->context_->preview_pane_->initWebView(doc);
              webkit_policy_decision_ignore(decision);
              g_free(filename);
              return;
            }
          }

          g_free(filename);
          webkit_policy_decision_use(decision);
          return;
        }
      }

      // let WebKit navigate to external links normally
      webkit_policy_decision_use(decision);
      return;
    }

    default:
      // default behavior for other policy types
      webkit_policy_decision_use(decision);
      return;
  }
}

gboolean WebView::onScrollEvent(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
  auto *self = static_cast<WebView *>(user_data);

  if (self->context_ && self->context_->preview_config_) {
    if (self->context_->preview_config_->get<bool>("disable_preview_ctrl_wheel_zoom", false)) {
      return false;
    }
  }

  if ((event->state & GDK_CONTROL_MASK) == 0) {
    return false;
  }

  if (event->direction == GDK_SCROLL_SMOOTH) {
    double zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(self->webview_));
    zoom = std::max(zoom - event->delta_y * 0.1, 0.01);
    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(self->webview_), zoom);
  } else if (event->direction == GDK_SCROLL_UP) {
    self->stepZoom(+1);
  } else if (event->direction == GDK_SCROLL_DOWN) {
    self->stepZoom(-1);
  } else {
    return false;
  }

  return true;
}

WebView &
WebView::getDomSnapshot(std::string_view root_id, std::function<void(std::string)> callback) {
  auto *cb_ptr = new std::function<void(std::string)>(std::move(callback));

  std::string js;
  if (!root_id.empty()) {
    js = "var el = document.getElementById('" + escapeForJsTemplateLiteral(root_id) +
         "'); el ? el.innerHTML : '';";
  } else {
    js = "document.body.outerHTML;";
  }

  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(webview_),
      js.c_str(),
      -1,
      nullptr,
      nullptr,
      nullptr,
      [](GObject *source, GAsyncResult *res, gpointer user_data) {
        auto *cb = static_cast<std::function<void(std::string)> *>(user_data);
        std::string html;
        GError *err = nullptr;
        JSCValue *val =
            webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), res, &err);

        if (!err && jsc_value_is_string(val)) {
          gchar *str = jsc_value_to_string(val);
          if (str) {
            html.assign(str);
            g_free(str);
          }
        }
        if (G_IS_OBJECT(val)) {
          g_object_unref(val);
        }
        if (err) {
          g_error_free(err);
        }

        (*cb)(html);
        delete cb;
      },
      cb_ptr
  );

  return *this;
}

namespace {
constexpr int BASE_FONT_SIZE = 12;
constexpr int SCINTILLA_MIN_OFFSET = -10;
constexpr int SCINTILLA_MAX_OFFSET = 20;
}  // namespace

WebView &WebView::resetZoom() {
  double zoom = 1.0;
  if (context_ && context_->preview_config_) {
    zoom = context_->preview_config_->get<double>("preview_zoom_default", 1.0);
  }
  return setZoom(zoom);
}

WebView &WebView::setZoom(double zoom) {
  constexpr double min_factor =
      static_cast<double>(std::max(BASE_FONT_SIZE + SCINTILLA_MIN_OFFSET, 1)) / BASE_FONT_SIZE;
  constexpr double max_factor =
      static_cast<double>(BASE_FONT_SIZE + SCINTILLA_MAX_OFFSET) / BASE_FONT_SIZE;

  zoom = std::clamp(zoom, min_factor, max_factor);
  webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webview_), zoom);
  return *this;
}

WebView &WebView::stepZoom(int step) {
  double zoom_step = static_cast<double>(step) / BASE_FONT_SIZE;
  double zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(webview_));
  return setZoom(zoom + zoom_step);
}
