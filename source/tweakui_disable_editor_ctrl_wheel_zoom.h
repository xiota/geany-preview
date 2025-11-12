// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <pluginutils.h>
#include <scintilla/Scintilla.h>

#include "preview_context.h"

class TweakUiDisableEditorCtrlWheelZoom {
 public:
  explicit TweakUiDisableEditorCtrlWheelZoom(PreviewContext *context) : context_(context) {
    if (!context_ || !context_->geany_data_) {
      return;
    }

    // Attach to already-open documents
    auto *docs = context_->geany_data_->documents_array;
    for (guint i = 0; i < docs->len; ++i) {
      auto *doc = static_cast<GeanyDocument *>(g_ptr_array_index(docs, i));
      if (DOC_VALID(doc)) {
        connectScrollHandler(doc);
      }
    }

    // Hook into new documents
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-open",
        true,
        G_CALLBACK(documentSignal),
        this
    );
    plugin_signal_connect(
        context_->geany_plugin_, nullptr, "document-new", true, G_CALLBACK(documentSignal), this
    );
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-close",
        false,
        G_CALLBACK(documentClose),
        this
    );
  }

 private:
  static gboolean onScrollEvent(GtkWidget *, GdkEventScroll *event, gpointer user_data) {
    auto *self = static_cast<TweakUiDisableEditorCtrlWheelZoom *>(user_data);
    if (!self->context_ || !self->context_->preview_config_) {
      return false;
    }

    if (!self->context_->preview_config_->get<bool>("disable_editor_ctrl_wheel_zoom", true)) {
      return false;
    }

    if ((event->state & GDK_CONTROL_MASK) != 0) {
      // block zoom
      return true;
    }

    // allow normal scrolling
    return false;
  }

  void connectScrollHandler(GeanyDocument *doc) {
    g_return_if_fail(DOC_VALID(doc));
    plugin_signal_connect(
        context_->geany_plugin_,
        G_OBJECT(doc->editor->sci),
        "scroll-event",
        false,
        G_CALLBACK(onScrollEvent),
        this
    );
  }

  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiDisableEditorCtrlWheelZoom *>(user_data);
    self->connectScrollHandler(doc);
  }

  static void documentClose(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiDisableEditorCtrlWheelZoom *>(user_data);
    g_return_if_fail(DOC_VALID(doc));
    g_signal_handlers_disconnect_by_func(doc->editor->sci, gpointer(onScrollEvent), self);
  }

  PreviewContext *context_ = nullptr;
};
