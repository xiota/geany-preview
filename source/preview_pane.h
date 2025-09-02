// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>

#include <gtk/gtk.h>

#include "converter_registrar.h"
#include "preview_config.h"
#include "preview_context.h"
#include "util/gtk_helpers.h"
#include "webview.h"

class PreviewPane final {
 public:
  explicit PreviewPane(PreviewContext *context)
      : context_(context),
        sidebar_notebook_(context_->geany_sidebar_),
        preview_config_(context_->preview_config_) {
    context_->webview_ = &webview_;
    attach();
  }

  ~PreviewPane() noexcept {
    detach();
  }

  PreviewPane &attach() {
    // Create a page container for the notebook
    // and an offscreen window to keep the WebView mapped
    page_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    offscreen_ = gtk_offscreen_window_new();
    gtk_widget_show(offscreen_);  // offscreen must be shown so its child is mapped

    // Add page to notebook
    if (GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      // Initially put the WebView in the page container
      gtk_box_pack_start(GTK_BOX(page_box_), webview_.widget(), TRUE, TRUE, 0);

      sidebar_page_number_ = gtk_notebook_append_page(
          GTK_NOTEBOOK(sidebar_notebook_), page_box_, gtk_label_new("Preview")
      );
      gtk_widget_show_all(page_box_);

      // Make the preview page the current page
      gtk_notebook_set_current_page(GTK_NOTEBOOK(sidebar_notebook_), sidebar_page_number_);
    }

    // Connect before first load so we don't miss it
    init_handler_id_ = g_signal_connect(
        webview_.widget(),
        "load-changed",
        G_CALLBACK(+[](WebKitWebView *, WebKitLoadEvent e, gpointer user_data) {
          auto *self = static_cast<PreviewPane *>(user_data);
          if (e == WEBKIT_LOAD_FINISHED) {
            self->triggerUpdate();  // one-time startup update
            g_signal_handler_disconnect(self->webview_.widget(), self->init_handler_id_);
            self->init_handler_id_ = 0;
          }
        }),
        this
    );

    // Initial template
    webview_.loadHtml("");

    // Connect switch-page once
    sidebar_switch_page_handler_id_ = g_signal_connect(
        sidebar_notebook_,
        "switch-page",
        G_CALLBACK(+[](GtkNotebook *notebook,
                       GtkWidget *page,
                       guint /*page_num*/,
                       gpointer user_data) {
          auto *self = static_cast<PreviewPane *>(user_data);

          if (page == self->page_box_) {
            // Entering preview: reparent from offscreen -> page
            self->safeReparentWebView_(self->page_box_, /*pack_into_box=*/true);
            self->triggerUpdate();
          } else {
            // Leaving preview: keep WebView mapped by moving it into the offscreen window
            // Try to keep offscreen size in sync to minimize reflow/relayout
            int w = gtk_widget_get_allocated_width(self->page_box_);
            int h = gtk_widget_get_allocated_height(self->page_box_);
            if (w > 0 && h > 0) {
              gtk_window_resize(GTK_WINDOW(self->offscreen_), w, h);
            }
            self->safeReparentWebView_(self->offscreen_, /*pack_into_box=*/false);
          }
        }),
        this
    );

    return *this;
  }

  PreviewPane &detach() {
    // Remove our page from the notebook
    if (page_box_ && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      int idx = gtk_notebook_page_num(GTK_NOTEBOOK(sidebar_notebook_), page_box_);
      if (idx >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(sidebar_notebook_), idx);
      }
    }

    // Ensure WebView is unparented before destroying containers
    if (GtkWidget *wv = webview_.widget()) {
      if (GTK_IS_WIDGET(wv)) {
        if (GtkWidget *p = gtk_widget_get_parent(wv)) {
          gtk_container_remove(GTK_CONTAINER(p), wv);
        }
      }
    }

    // Disconnect signals
    if (sidebar_switch_page_handler_id_) {
      g_signal_handler_disconnect(sidebar_notebook_, sidebar_switch_page_handler_id_);
      sidebar_switch_page_handler_id_ = 0;
    }

    // Destroy helper widgets
    if (offscreen_) {
      gtk_widget_destroy(offscreen_);
      offscreen_ = nullptr;
    }
    if (page_box_) {
      gtk_widget_destroy(page_box_);
      page_box_ = nullptr;
    }

    return *this;
  }

  // For visibility checks in helpers
  GtkWidget *widget() const {
    return page_box_ ? page_box_ : webview_.widget();
  }

  PreviewPane &scheduleUpdate() {
    // Skip if update already running or scheduled
    if (update_pending_) {
      return *this;
    }

    update_pending_ = true;  // Reserve the slot now.

    gint64 now = g_get_monotonic_time() / 1000;  // ms
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
    // update_pending_ is true from scheduling.
    update();
    last_update_time_ = g_get_monotonic_time() / 1000;
    update_pending_ = false;  // Free slot for next schedule.
  }

 private:
  // Safely reparent the webview between containers, keeping it alive across the transition.
  void safeReparentWebView_(GtkWidget *new_parent, bool pack_into_box) {
    GtkWidget *wv = webview_.widget();
    if (!GTK_IS_WIDGET(new_parent) || !GTK_IS_WIDGET(wv)) {
      return;
    }

    g_object_ref(wv);  // keep alive across remove/add

    if (GtkWidget *old_parent = gtk_widget_get_parent(wv)) {
      gtk_container_remove(GTK_CONTAINER(old_parent), wv);
    }

    // If new_parent is a bin-like (offscreen window), ensure it has no other child
    if (GTK_IS_BIN(new_parent)) {
      GtkWidget *child = gtk_bin_get_child(GTK_BIN(new_parent));
      if (child && child != wv) {
        gtk_container_remove(GTK_CONTAINER(new_parent), child);
      }
    }

    if (pack_into_box && GTK_IS_BOX(new_parent)) {
      gtk_box_pack_start(GTK_BOX(new_parent), wv, TRUE, TRUE, 0);
    } else {
      gtk_container_add(GTK_CONTAINER(new_parent), wv);
    }

    gtk_widget_show(wv);
    g_object_unref(wv);  // new parent owns the reference now
  }

  PreviewPane &update() {
    Document document(document_get_current());
    auto *converter = registrar_.getConverter(document);

    if (converter) {
      auto file = document.fileName();
      auto html = converter->toHtml(document.textView());
      webview_.getScrollFraction([this, file, html](double frac) {
        scroll_by_file_[file] = frac;
        webview_.updateHtml(html, &scroll_by_file_[file]);
      });
    } else {
      auto text = "<tt>" + document.filetypeName() + ", " + document.encodingName() + "</tt>";
      webview_.updateHtml(text);
    }

    return *this;
  }

 private:
  PreviewContext *context_;
  gulong init_handler_id_ = 0;

  GtkWidget *sidebar_notebook_;
  GtkWidget *page_box_ = nullptr;   // notebook page container
  GtkWidget *offscreen_ = nullptr;  // keeps WebView mapped when page is hidden
  guint sidebar_page_number_ = 0;
  gulong sidebar_switch_page_handler_id_ = 0;

  PreviewConfig *preview_config_;

  WebView webview_;
  ConverterRegistrar registrar_;

  bool update_pending_ = false;
  gint64 last_update_time_ = 0;

  std::unordered_map<std::string, double> scroll_by_file_;
};
