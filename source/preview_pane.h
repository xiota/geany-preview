// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>

#include <gtk/gtk.h>

#include "converter_registrar.h"
#include "preview_config.h"
#include "preview_context.h"
#include "util/file_utils.h"
#include "util/gtk_utils.h"
#include "webview.h"

class PreviewPane final {
 public:
  explicit PreviewPane(PreviewContext *context)
      : context_(context),
        sidebar_notebook_(context_->geany_sidebar_),
        preview_config_(context_->preview_config_) {
    context_->webview_ = &webview_;

    page_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    offscreen_ = gtk_offscreen_window_new();
    gtk_widget_show(offscreen_);

    if (GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      gtk_box_pack_start(GTK_BOX(page_box_), webview_.widget(), true, true, 0);

      sidebar_page_number_ = gtk_notebook_append_page(
          GTK_NOTEBOOK(sidebar_notebook_), page_box_, gtk_label_new("Preview")
      );
      gtk_widget_show_all(page_box_);
      gtk_notebook_set_current_page(GTK_NOTEBOOK(sidebar_notebook_), sidebar_page_number_);

      // for styling with geany.css
      gtk_widget_set_name(GTK_WIDGET(page_box_), "geany-preview-sidebar-page");

      // simulated :focus-within tracking for CSS styling
      GtkUtils::enableFocusWithinTracking(page_box_);
    }

    initWebView();

    init_handler_id_ = g_signal_connect(
        webview_.widget(),
        "load-changed",
        G_CALLBACK(+[](WebKitWebView *, WebKitLoadEvent e, gpointer user_data) {
          auto *self = static_cast<PreviewPane *>(user_data);
          if (e == WEBKIT_LOAD_FINISHED) {
            self->scheduleUpdate();
            g_signal_handler_disconnect(self->webview_.widget(), self->init_handler_id_);
            self->init_handler_id_ = 0;
          }
        }),
        this
    );

    g_signal_connect(
        webview_.widget(),
        "load-changed",
        G_CALLBACK(+[](WebKitWebView *, WebKitLoadEvent e, gpointer user_data) {
          auto *self = static_cast<PreviewPane *>(user_data);
          if (e == WEBKIT_LOAD_FINISHED) {
            self->checkHealth();
          }
        }),
        this
    );

    sidebar_switch_page_handler_id_ = g_signal_connect(
        sidebar_notebook_,
        "switch-page",
        G_CALLBACK(+[](GtkNotebook *notebook,
                       GtkWidget *page,
                       guint /*page_num*/,
                       gpointer user_data) {
          auto *self = static_cast<PreviewPane *>(user_data);

          if (page == self->page_box_) {
            self->safeReparentWebView_(self->page_box_, true);
            self->triggerUpdate();
          } else {
            int w = gtk_widget_get_allocated_width(self->page_box_);
            int h = gtk_widget_get_allocated_height(self->page_box_);
            if (w > 0 && h > 0) {
              gtk_window_resize(GTK_WINDOW(self->offscreen_), w, h);
            }
            self->safeReparentWebView_(self->offscreen_, false);
          }
        }),
        this
    );
  }

  ~PreviewPane() noexcept {
    if (page_box_ && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      int idx = gtk_notebook_page_num(GTK_NOTEBOOK(sidebar_notebook_), page_box_);
      if (idx >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(sidebar_notebook_), idx);
      }
    }

    if (GtkWidget *wv = webview_.widget()) {
      if (GTK_IS_WIDGET(wv)) {
        if (GtkWidget *p = gtk_widget_get_parent(wv)) {
          gtk_container_remove(GTK_CONTAINER(p), wv);
        }
      }
    }

    if (sidebar_switch_page_handler_id_) {
      g_signal_handler_disconnect(sidebar_notebook_, sidebar_switch_page_handler_id_);
      sidebar_switch_page_handler_id_ = 0;
    }

    if (GTK_IS_WIDGET(offscreen_)) {
      gtk_widget_destroy(offscreen_);
      offscreen_ = nullptr;
    }

    stopAllWatches();
  }

  GtkWidget *widget() const {
    return page_box_ ? page_box_ : webview_.widget();
  }

  void initWebView() {
    previous_base_uri_ = std::filesystem::path{};
    previous_key_.clear();
    previous_theme_.clear();

    webview_.loadHtml("");

    webview_.clearInjectedCss();
    addWatchIfNeeded(preview_config_->configDir() / "preview.css");
    webview_.injectCssFromFile(preview_config_->configDir() / "preview.css");

    auto theme = preview_config_->get<std::string>("theme_mode", "system");
    if (theme == "light") {
      webview_.injectCssFromString("html { color-scheme: light; }");
    } else if (theme == "dark") {
      webview_.injectCssFromString("html { color-scheme: dark; }");
    }

    triggerUpdate();
  }

  PreviewPane &checkHealth() {
    const char *js = R"JS(
!(
  document.documentElement &&
  document.head &&
  document.body &&
  document.getElementById('root')
)
)JS";

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webview_.widget()),
        js,
        -1,
        nullptr,
        nullptr,
        nullptr,
        [](GObject *source, GAsyncResult *res, gpointer user_data) {
          GError *err = nullptr;
          JSCValue *val =
              webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), res, &err);
          bool needs_reinit = false;
          if (!err && jsc_value_is_boolean(val)) {
            needs_reinit = jsc_value_to_boolean(val);
          }
          if (G_IS_OBJECT(val)) {
            g_object_unref(val);
          }
          if (err) {
            g_error_free(err);
          }

          if (needs_reinit) {
            auto *self = static_cast<PreviewPane *>(user_data);
            self->initWebView();
          }
        },
        this
    );
    return *this;
  }

  PreviewPane &scheduleUpdate() {
    if (update_pending_) {
      return *this;
    }

    update_pending_ = true;

    gint64 now = g_get_monotonic_time() / 1000;
    gint64 update_cooldown_ms_ = preview_config_->get<int>("update_cooldown");
    gint64 update_min_delay_ = preview_config_->get<int>("update_min_delay");

    gint64 delay_ms =
        std::max(update_min_delay_, update_cooldown_ms_ - (now - last_update_time_));

    g_timeout_add(
        delay_ms,
        [](gpointer data) -> gboolean {
          auto *self = static_cast<PreviewPane *>(data);
          self->triggerUpdate();
          return G_SOURCE_REMOVE;
        },
        this
    );
    return *this;
  }

  void triggerUpdate() {
    update();
    last_update_time_ = g_get_monotonic_time() / 1000;
    update_pending_ = false;
  }

 private:
  void safeReparentWebView_(GtkWidget *new_parent, bool pack_into_box) {
    GtkWidget *wv = webview_.widget();
    if (!GTK_IS_WIDGET(new_parent) || !GTK_IS_WIDGET(wv)) {
      return;
    }

    g_object_ref(wv);

    if (GtkWidget *old_parent = gtk_widget_get_parent(wv)) {
      gtk_container_remove(GTK_CONTAINER(old_parent), wv);
    }

    if (GTK_IS_BIN(new_parent)) {
      GtkWidget *child = gtk_bin_get_child(GTK_BIN(new_parent));
      if (child && child != wv) {
        gtk_container_remove(GTK_CONTAINER(new_parent), child);
      }
    }

    if (pack_into_box && GTK_IS_BOX(new_parent)) {
      gtk_box_pack_start(GTK_BOX(new_parent), wv, true, true, 0);
    } else {
      gtk_container_add(GTK_CONTAINER(new_parent), wv);
    }

    gtk_widget_show(wv);
    g_object_unref(wv);
  }

  PreviewPane &update() {
    Document document(document_get_current());
    auto *converter = registrar_.getConverter(document);

    std::string html;
    if (converter) {
      html = converter->toHtml(document.textView());  // implicit string_view→string
    } else {
      html = "<tt>" + document.filetypeName() + ", " + document.encodingName() + "</tt>";
    }

    // load new css on document type change
    bool changed = false;
    auto key = registrar_.getConverterKey(document);
    if (key != previous_key_) {
      addWatchIfNeeded(preview_config_->configDir() / "preview.css");
      addWatchIfNeeded(preview_config_->configDir() / std::string{ key + ".css" });
      webview_.clearInjectedCss();
      webview_.injectCssFromFile(preview_config_->configDir() / "preview.css");
      webview_.injectCssFromFile(preview_config_->configDir() / std::string{ key + ".css" });
      previous_key_ = key;
      changed = true;
    }

    auto theme = preview_config_->get<std::string>("theme_mode", "system");
    if (changed || theme != previous_theme_) {
      if (theme == "light") {
        webview_.injectCssFromString("html { color-scheme: light; }");
      } else if (theme == "dark") {
        webview_.injectCssFromString("html { color-scheme: dark; }");
      } else {
        webview_.injectCssFromString("html { color-scheme: light dark; }");
      }
      previous_theme_ = theme;
    }

    // base URI
    auto file = document.fileName();
    auto file_path = std::filesystem::path(file);
    auto current_dir = file_path.parent_path().lexically_normal();

    std::string base_uri_str;
    if (current_dir != previous_base_uri_) {
      if (!current_dir.empty()) {
        base_uri_str = "file://" + current_dir.string() + "/";
      }
      previous_base_uri_ = current_dir;
    }

    // inject HTML
    bool allow_fallback = preview_config_->get<bool>("allow_update_fallback", false);
    webview_.getScrollFraction([this, file, base_uri_str, html, allow_fallback](double frac) {
      scroll_by_file_[file] = frac;
      webview_.updateHtml(html, base_uri_str, &scroll_by_file_[file], allow_fallback);
    });

    return *this;
  }

  void addWatchIfNeeded(const std::filesystem::path &path) {
    if (watches_.find(path) != watches_.end()) {
      return;
    }

    // Construct the handle in-place in the map
    auto [it, inserted] = watches_.emplace(path, FileUtils::FileWatchHandle{});

    FileUtils::watchFile(it->second, path, [this, path]() {
      bool changed = false;
      if (path == preview_config_->configDir() / (previous_key_ + ".css")) {
        webview_.clearInjectedCss();
        webview_.injectCssFromFile(preview_config_->configDir() / "preview.css");
        webview_.injectCssFromFile(path);
        changed = true;
      } else if (path == preview_config_->configDir() / "preview.css") {
        webview_.clearInjectedCss();
        webview_.injectCssFromFile(path);
        webview_.injectCssFromFile(preview_config_->configDir() / (previous_key_ + ".css"));
        changed = true;
      }
      if (changed) {
        auto theme = preview_config_->get<std::string>("theme_mode", "system");
        if (theme == "light") {
          webview_.injectCssFromString("html { color-scheme: light; }");
        } else if (theme == "dark") {
          webview_.injectCssFromString("html { color-scheme: dark; }");
        } else {
          webview_.injectCssFromString("html { color-scheme: light dark; }");
        }
      }
    });
  }

  void stopAllWatches() {
    for (auto &[path, handle] : watches_) {
      FileUtils::stopWatching(handle);
    }
    watches_.clear();
  }

 private:
  PreviewContext *context_;
  gulong init_handler_id_ = 0;

  GtkWidget *sidebar_notebook_;
  GtkWidget *page_box_ = nullptr;
  GtkWidget *offscreen_ = nullptr;
  guint sidebar_page_number_ = 0;
  gulong sidebar_switch_page_handler_id_ = 0;

  PreviewConfig *preview_config_;

  WebView webview_;
  ConverterRegistrar registrar_;

  bool update_pending_ = false;
  gint64 last_update_time_ = 0;

  std::unordered_map<std::string, double> scroll_by_file_;
  std::string previous_key_;
  std::string previous_theme_;

  std::unordered_map<std::filesystem::path, FileUtils::FileWatchHandle> watches_;
  std::filesystem::path previous_base_uri_;
};
