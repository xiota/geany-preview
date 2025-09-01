// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

#include <geanyplugin.h>
#include <gtk/gtk.h>

#include "converter_registrar.h"
#include "preview_config.h"
#include "webview.h"

class PreviewPane final {
 public:
  PreviewPane(GtkWidget *notebook, PreviewConfig *config)
      : sidebar_notebook_(notebook), preview_config_(config) {
    attach();
  }

  ~PreviewPane() noexcept {
    detach();
  }

  PreviewPane &attach() {
    auto *webview_widget = webview_.widget();
    if (webview_widget && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      sidebar_page_number_ = gtk_notebook_append_page(
          GTK_NOTEBOOK(sidebar_notebook_), webview_widget, gtk_label_new("Preview")
      );
      gtk_widget_show_all(webview_widget);
      gtk_notebook_set_current_page(GTK_NOTEBOOK(sidebar_notebook_), sidebar_page_number_);
    }
    webview_.loadHtml("");  // load html template
    update();
    return *this;
  }

  PreviewPane &detach() {
    auto *webview_widget = webview_.widget();
    if (webview_widget && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      int idx = gtk_notebook_page_num(GTK_NOTEBOOK(sidebar_notebook_), webview_widget);
      if (idx >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(sidebar_notebook_), idx);
      }
    }
    return *this;
  }

  PreviewPane &scheduleUpdate() {
    // If an update is already running OR a timer is already scheduled, do nothing.
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

 private:
  void triggerUpdate() {
    // update_pending_ is true from scheduling.
    update();
    last_update_time_ = g_get_monotonic_time() / 1000;
    update_pending_ = false;  // Free slot for next schedule.
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
  GtkWidget *sidebar_notebook_;
  int sidebar_page_number_ = 0;

  PreviewConfig *preview_config_;

  WebView webview_;
  ConverterRegistrar registrar_;

  bool update_pending_ = false;
  gint64 last_update_time_ = 0;

  std::unordered_map<std::string, double> scroll_by_file_;
};
