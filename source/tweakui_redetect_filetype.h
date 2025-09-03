// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <pluginutils.h>

#include "preview_context.h"

class TweakUiRedetectFileType {
 public:
  explicit TweakUiRedetectFileType(PreviewContext *context) : context_(context) {
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-reload",
        true,
        G_CALLBACK(documentSignal),
        this
    );
  }

  void redetectFileType(GeanyDocument *doc = nullptr) {
    if (!DOC_VALID(doc)) {
      doc = document_get_current();
    }
    if (!DOC_VALID(doc)) {
      return;
    }

    bool keep_type = false;
    if (doc->file_type->id != GEANY_FILETYPES_NONE) {
      // Don't change filetype if filename matches pattern.
      // Avoids changing *.h files to C after they have been changed to C++.
      char **pattern = doc->file_type->pattern;
      for (int i = 0; pattern[i] != nullptr; ++i) {
        if (g_pattern_match_simple(pattern[i], doc->file_name)) {
          keep_type = true;
          break;
        }
      }
    }

    if (!keep_type) {
      auto new_type = filetypes_detect_from_file(doc->file_name);
      if (new_type->id != GEANY_FILETYPES_NONE) {
        document_set_filetype(doc, new_type);
      }
    }
  }

  void forceRedetectFileType(GeanyDocument *doc = nullptr) {
    if (!DOC_VALID(doc)) {
      doc = document_get_current();
    }
    if (!DOC_VALID(doc)) {
      return;
    }
    document_set_filetype(doc, filetypes_detect_from_file(doc->file_name));
  }

 private:
  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiRedetectFileType *>(user_data);

    if (!DOC_VALID(doc) || !doc->file_name || !self->context_ ||
        !self->context_->preview_config_->get<bool>("redetect_filetype_on_reload", false)) {
      return;
    }

    self->redetectFileType(doc);
  }

  PreviewContext *context_ = nullptr;
};
