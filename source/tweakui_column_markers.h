// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <scintilla/Scintilla.h>
#include <pluginutils.h>  // for plugin_signal_connect()

#include "preview_config.h"
#include "preview_context.h"

class TweakUiColumnMarkers {
 public:
  explicit TweakUiColumnMarkers(PreviewContext *ctx) : context_(ctx) {
    auto *docs = context_->geany_data_->documents_array;
    for (guint i = 0; i < docs->len; ++i) {
      auto *doc = static_cast<GeanyDocument *>(g_ptr_array_index(docs, i));
      if (DOC_VALID(doc)) {
        show(doc);
      }
    }

    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-activate",
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
        "document-open",
        true,
        G_CALLBACK(documentSignal),
        this
    );
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-reload",
        true,
        G_CALLBACK(documentSignal),
        this
    );
  }

  void show(GeanyDocument *doc = nullptr) {
    if (!context_->preview_config_->get<bool>("column_markers", false)) {
      clear();
      return;
    }

    if (!DOC_VALID(doc)) {
      doc = document_get_current();
      g_return_if_fail(DOC_VALID(doc));
    }

    auto cols = context_->preview_config_->get<std::vector<int>>(
        "column_markers_columns", {60, 72, 80, 88, 96, 104, 112, 120, 128}
    );
    auto colors = context_->preview_config_->get<std::vector<std::string>>(
        "column_markers_colors",
        {"#e5e5e5",
         "#b0d0ff",
         "#ffc0ff",
         "#e5e5e5",
         "#ffb0a0",
         "#e5e5e5",
         "#e5e5e5",
         "#e5e5e5",
         "#e5e5e5"}
    );

    const auto count = std::min(cols.size(), colors.size());

    scintilla_send_message(doc->editor->sci, SCI_SETEDGEMODE, 3, 3);
    scintilla_send_message(doc->editor->sci, SCI_MULTIEDGECLEARALL, 0, 0);

    for (size_t i = 0; i < count; ++i) {
      if (cols[i] <= 0 || colors[i].empty()) {
        continue;
      }
      scintilla_send_message(
          doc->editor->sci, SCI_MULTIEDGEADDLINE, cols[i], parseColorString(colors[i])
      );
    }
  }

  void clear(GeanyDocument *doc = nullptr) {
    if (!DOC_VALID(doc)) {
      doc = document_get_current();
      g_return_if_fail(DOC_VALID(doc));
    }

    scintilla_send_message(doc->editor->sci, SCI_SETEDGEMODE, EDGE_NONE, 0);
    scintilla_send_message(doc->editor->sci, SCI_MULTIEDGECLEARALL, 0, 0);
  }

 private:
  static void documentSignal(GObject *, GeanyDocument *, gpointer user_data) {
    auto *self = static_cast<TweakUiColumnMarkers *>(user_data);

    if (!self->show_idle_in_progress_) {
      self->show_idle_in_progress_ = true;
      g_idle_add_full(
          G_PRIORITY_DEFAULT_IDLE,
          +[](gpointer data) -> gboolean {
            auto *me = static_cast<TweakUiColumnMarkers *>(data);
            me->show();
            me->show_idle_in_progress_ = false;
            return G_SOURCE_REMOVE;
          },
          self,
          nullptr
      );
    }
  }

  static int parseColorString(const std::string &hex) {
    if (hex.empty()) {
      return 0;
    }
    auto hexToInt = [](char c) -> int {
      if (c >= '0' && c <= '9') {
        return c - '0';
      }
      if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
      }
      if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
      }
      return 0;
    };
    if (hex[0] == '#' && hex.size() == 7) {
      int r = hexToInt(hex[1]) * 16 + hexToInt(hex[2]);
      int g = hexToInt(hex[3]) * 16 + hexToInt(hex[4]);
      int b = hexToInt(hex[5]) * 16 + hexToInt(hex[6]);
      return (b << 16) | (g << 8) | r;  // BGR for Scintilla
    } else if (hex[0] == '#' && hex.size() == 4) {
      int r = hexToInt(hex[1]) * 16 + hexToInt(hex[1]);
      int g = hexToInt(hex[2]) * 16 + hexToInt(hex[2]);
      int b = hexToInt(hex[3]) * 16 + hexToInt(hex[3]);
      return (b << 16) | (g << 8) | r;  // BGR for Scintilla
    }

    return 0;
  }

  PreviewContext *context_;
  bool show_idle_in_progress_ = false;
};
