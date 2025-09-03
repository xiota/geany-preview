// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdarg>
#include <string>
#include <vector>

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <msgwindow.h>
#include <pluginutils.h>
#include <scintilla/Scintilla.h>

#include "preview_config.h"
#include "preview_context.h"

class TweakUiSidebarAutoResize;

namespace {
struct DelayedApplyData {
  TweakUiSidebarAutoResize *self;
  int target_cols;
  bool paned_left;
};
}  // anonymous namespace

class TweakUiSidebarAutoResize {
 public:
  explicit TweakUiSidebarAutoResize(PreviewContext *context) : context_(context) {
    if (!context_ || !context_->geany_data_ || !context_->geany_data_->main_widgets) {
      return;
    }

    geany_window_ = context_->geany_data_->main_widgets->window;
    geany_sidebar_ = context_->geany_data_->main_widgets->sidebar_notebook;
    geany_hpane_ = ui_lookup_widget(GTK_WIDGET(geany_window_), "hpaned1");

    if (!geany_window_ || !geany_sidebar_ || !geany_hpane_) {
      return;
    }

    g_signal_connect(geany_window_, "window-state-event", G_CALLBACK(onWindowEvent), this);
    g_signal_connect(geany_window_, "configure-event", G_CALLBACK(onWindowEvent), this);

    // Defer initial adjustment until idle
    g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        +[](gpointer data) -> gboolean {
          static_cast<TweakUiSidebarAutoResize *>(data)->adjustSidebar();
          return G_SOURCE_REMOVE;
        },
        this,
        nullptr
    );
  }

 private:
  static gboolean onWindowEvent(GtkWidget *, GdkEvent *, gpointer user_data) {
    auto *self = static_cast<TweakUiSidebarAutoResize *>(user_data);
    self->adjustSidebar();
    return false;
  }

  void debugLog(const char *fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    char *msg = g_strdup_vprintf(fmt, args);
    va_end(args);

    if (msg) {
      msgwin_status_add("%s", msg);
      g_free(msg);
    }
  }

  int calculateTargetWidth(int columns) const {
    GeanyDocument *doc = document_get_current();
    if (!DOC_VALID(doc) || columns <= 0) {
      return -1;
    }
    if (!doc->editor || !doc->editor->sci) {
      return -1;
    }
    std::string str(columns, '0');
    int pos_origin = static_cast<int>(
        scintilla_send_message(doc->editor->sci, SCI_POINTXFROMPOSITION, 0, 1)
    );
    return pos_origin +
           static_cast<int>(scintilla_send_message(
               doc->editor->sci, SCI_TEXTWIDTH, 0, reinterpret_cast<sptr_t>(str.c_str())
           ));
  }

  void adjustSidebar() {
    // Core widget/type guards (cheap, before we set pending)
    if (!geany_hpane_ || !geany_window_ || !geany_sidebar_) {
      return;
    }

    bool enabled = context_->preview_config_->get<bool>("sidebar_auto_resize", false);
    if (!enabled) {
      return;
    }

    if (adjustment_pending_) {
      return;
    }
    adjustment_pending_ = true;

    // Runtime readiness guards (may be false during early startup)
    GdkWindow *gwin = gtk_widget_get_window(geany_window_);
    if (!gwin) {
      adjustment_pending_ = false;
      return;
    }
    if (!gtk_widget_get_mapped(geany_hpane_)) {
      // Paned not mapped yet; no valid allocation to work with
      adjustment_pending_ = false;
      return;
    }
    int paned_w_now = gtk_widget_get_allocated_width(geany_hpane_);
    if (paned_w_now <= 0) {
      adjustment_pending_ = false;
      return;
    }

    bool paned_left = (geany_sidebar_ == gtk_paned_get_child1(GTK_PANED(geany_hpane_)));

    GdkWindowState wstate = gdk_window_get_state(gwin);
    bool fullscreen = (wstate & GDK_WINDOW_STATE_FULLSCREEN);
    bool maximized = (wstate & GDK_WINDOW_STATE_MAXIMIZED);
    if (fullscreen) {
      maximized = false;
    }

    int cols_normal = context_->preview_config_->get<int>("sidebar_columns_normal", 80);
    int cols_maximized = context_->preview_config_->get<int>("sidebar_columns_maximized", 100);
    int cols_fullscreen =
        context_->preview_config_->get<int>("sidebar_columns_fullscreen", 100);

    int target_cols = cols_normal;
    if (fullscreen) {
      target_cols = cols_fullscreen;
    } else if (maximized) {
      target_cols = cols_maximized;
    }

    int delay_ms = context_->preview_config_->get<int>("sidebar_resize_delay", 50);
    if (delay_ms < 0) {
      delay_ms = 0;
    }

    auto *data = g_new0(DelayedApplyData, 1);
    data->self = this;
    data->target_cols = target_cols;
    data->paned_left = paned_left;

    g_timeout_add_full(
        G_PRIORITY_LOW,
        delay_ms,
        [](gpointer user_data) -> gboolean {
          auto *d = static_cast<DelayedApplyData *>(user_data);

          // Re-check runtime readiness inside the timeout as well
          if (!d->self->geany_hpane_ || !GTK_IS_PANED(d->self->geany_hpane_) ||
              !gtk_widget_get_mapped(d->self->geany_hpane_)) {
            d->self->adjustment_pending_ = false;
            g_free(d);
            return G_SOURCE_REMOVE;
          }

          int paned_w = gtk_widget_get_allocated_width(d->self->geany_hpane_);
          if (paned_w <= 0) {
            d->self->adjustment_pending_ = false;
            g_free(d);
            return G_SOURCE_REMOVE;
          }

          int target_editor_width = d->self->calculateTargetWidth(d->target_cols);
          if (target_editor_width < 0) {
            d->self->adjustment_pending_ = false;
            g_free(d);
            return G_SOURCE_REMOVE;
          }

          int min_sidebar_width =
              d->self->context_->preview_config_->get<int>("sidebar_size_min", 50);
          int max_editor_width = paned_w - min_sidebar_width;

          if (target_editor_width < kEditorMinWidth) {
            target_editor_width = kEditorMinWidth;
          }
          if (target_editor_width > max_editor_width) {
            target_editor_width = max_editor_width;
          }

          if (d->paned_left) {
            int pos = paned_w - target_editor_width;
            gtk_paned_set_position(GTK_PANED(d->self->geany_hpane_), pos);
          } else {
            int pos = target_editor_width;
            gtk_paned_set_position(GTK_PANED(d->self->geany_hpane_), pos);
          }

          d->self->adjustment_pending_ = false;
          g_free(d);
          return G_SOURCE_REMOVE;
        },
        data,
        nullptr
    );
  }

  PreviewContext *context_ = nullptr;
  GtkWidget *geany_window_ = nullptr;
  GtkWidget *geany_hpane_ = nullptr;
  GtkWidget *geany_sidebar_ = nullptr;

  bool adjustment_pending_ = false;
  static constexpr int kEditorMinWidth = 200;
};
