// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include <Scintilla.h>    // for SCNotification
#include <pluginutils.h>  // for plugin_signal_connect()

#include "preview_config.h"
#include "preview_context.h"

class TweakUiUnchangeDocument {
 public:
  explicit TweakUiUnchangeDocument(PreviewContext *ctx) : context_(ctx) {
    plugin_signal_connect(
        context_->geany_plugin_, nullptr, "editor-notify", false, G_CALLBACK(editorNotify), this
    );
  }

 private:
  static gboolean
  editorNotify(GObject *, GeanyEditor *editor, SCNotification *, gpointer user_data) {
    auto *self = static_cast<TweakUiUnchangeDocument *>(user_data);
    self->unchange(editor->document);
    return false;  // let Geany continue processing
  }

  void unchange(GeanyDocument *doc) {
    if (!DOC_VALID(doc) || !doc->changed) {
      return;
    }

    // Read enable flag from plugin config
    if (!context_->preview_config_->get<bool>("unchange_document", false)) {
      return;
    }

    // Only unmark if it's an untitled, empty buffer
    if (std::string_view(DOC_FILENAME(doc)) == std::string_view(GEANY_STRING_UNTITLED) &&
        sci_get_length(doc->editor->sci) == 0) {
      document_set_text_changed(doc, false);
    }
  }

  PreviewContext *context_;
};
