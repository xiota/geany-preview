// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
// SPDX-FileCopyrightText: Copyright 2009-2011 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <locale>
#include <string>

#include <glib.h>
#include <gtk/gtk.h>
#include <pluginutils.h>  // plugin_signal_connect()

#include "preview_config.h"
#include "preview_context.h"

class TweakUiMarkWord {
 public:
  explicit TweakUiMarkWord(PreviewContext *context) : context_(context) {
    // Connect the button-press event for open documents
    auto *docs = context_->geany_data_->documents_array;
    for (guint i = 0; i < docs->len; ++i) {
      auto *doc = static_cast<GeanyDocument *>(g_ptr_array_index(docs, i));
      if (DOC_VALID(doc)) {
        connectDocumentButtonPressSignalHandler(doc);
      }
    }

    // Hook into document and editor signals
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

    plugin_signal_connect(
        context_->geany_plugin_, nullptr, "editor-notify", false, G_CALLBACK(editorNotify), this
    );
  }

 private:
  bool isEnabled() const {
    return context_ && context_->preview_config_ &&
           context_->preview_config_->get<bool>("mark_word", false);
  }

  bool singleClickDeselect() const {
    return context_ && context_->preview_config_ &&
           context_->preview_config_->get<bool>("mark_word_single_click_deselect", true);
  }

  static void clearMarker(GeanyDocument *doc = nullptr) {
    if (!DOC_VALID(doc)) {
      doc = document_get_current();
    }
    if (DOC_VALID(doc)) {
      editor_indicator_clear(doc->editor, GEANY_INDICATOR_SEARCH);
    }
  }

  static gboolean markWord(gpointer user_data) {
    auto *self = static_cast<TweakUiMarkWord *>(user_data);
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_MARKALL);
    self->double_click_timer_id_ = 0;
    return false;
  }

  static gboolean
  onEditorButtonPressEvent(GtkWidget *, GdkEventButton *event, gpointer user_data) {
    auto *self = static_cast<TweakUiMarkWord *>(user_data);
    if (event->button == 1) {
      if (!self->isEnabled()) {
        return false;
      }
      if (event->type == GDK_BUTTON_PRESS) {
        if (self->singleClickDeselect()) {
          clearMarker();
        }
      } else if (event->type == GDK_2BUTTON_PRESS) {
        if (self->double_click_timer_id_ == 0) {
          if (self->context_ && self->context_->preview_config_) {
            int val =
                self->context_->preview_config_->get<int>("mark_word_double_click_delay", 50);
            self->double_click_delay_ms_ = val;
          }

          self->double_click_timer_id_ =
              g_timeout_add(self->double_click_delay_ms_, G_SOURCE_FUNC(markWord), self);
        }
      }
    }
    return false;
  }

  void connectDocumentButtonPressSignalHandler(GeanyDocument *doc) {
    g_return_if_fail(DOC_VALID(doc));
    plugin_signal_connect(
        context_->geany_plugin_,
        G_OBJECT(doc->editor->sci),
        "button-press-event",
        false,
        G_CALLBACK(onEditorButtonPressEvent),
        this
    );
  }

  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiMarkWord *>(user_data);
    self->connectDocumentButtonPressSignalHandler(doc);
  }

  static void documentClose(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiMarkWord *>(user_data);
    g_return_if_fail(DOC_VALID(doc));
    g_signal_handlers_disconnect_by_func(
        doc->editor->sci, gpointer(onEditorButtonPressEvent), self
    );
  }

  static bool
  editorNotify(GObject *, GeanyEditor *editor, SCNotification *nt, gpointer user_data) {
    auto *self = static_cast<TweakUiMarkWord *>(user_data);
    if (nt->nmhdr.code == SCN_MODIFIED &&
        ((nt->modificationType & SC_MOD_BEFOREDELETE) == SC_MOD_BEFOREDELETE) &&
        sci_has_selection(editor->sci)) {
      if (self->isEnabled() && self->singleClickDeselect()) {
        clearMarker(editor->document);
      }
    } else if (nt->nmhdr.code == SCN_UPDATEUI && nt->updated == SC_UPDATE_SELECTION &&
               !sci_has_selection(editor->sci)) {
      if (self->isEnabled() && self->singleClickDeselect()) {
        clearMarker(editor->document);
      }
    }
    return false;
  }

  PreviewContext *context_ = nullptr;
  gulong double_click_timer_id_ = 0;
  int double_click_delay_ms_ = 50;
};
