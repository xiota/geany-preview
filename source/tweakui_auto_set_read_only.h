// SPDX-FileCopyrightText: Copyright 2021, 2025-2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <locale>
#include <unistd.h>

#include <string>

#include <glib.h>
#include <gtk/gtk.h>
#include <pluginutils.h>  // for plugin_signal_connect()

#include "preview_config.h"
#include "preview_context.h"
#include "tweakui_base.h"

class TweakUiAutoSetReadOnly : public TweakUiBase<TweakUiAutoSetReadOnly> {
 public:
  explicit TweakUiAutoSetReadOnly() {
    auto &ctx = PreviewContext::instance();
    if (!ctx.geany_data_ || !ctx.geany_plugin_) {
      return;
    }

    if (ctx.geany_data_->main_widgets->window) {
      read_only_menu_item_ = GTK_CHECK_MENU_ITEM(ui_lookup_widget(
          GTK_WIDGET(ctx.geany_data_->main_widgets->window), "set_file_readonly1"
      ));
    }

    PreviewConfig::registerSetting(
        "tweakui/auto_set_read_only",
        false,
        "Automatically set documents to read-only mode when they are not writable."
    );

    plugin_signal_connect(
        ctx.geany_plugin_,
        nullptr,
        "document-activate",
        true,  // after
        G_CALLBACK(documentSignal),
        this
    );
  }

  void setReadOnly() {
    if (read_only_menu_item_) {
      gtk_check_menu_item_set_active(read_only_menu_item_, true);
    }
  }

  void unsetReadOnly() {
    if (read_only_menu_item_) {
      gtk_check_menu_item_set_active(read_only_menu_item_, false);
    }
  }

  void toggle() {
    if (read_only_menu_item_) {
      gtk_check_menu_item_set_active(
          read_only_menu_item_, !gtk_check_menu_item_get_active(read_only_menu_item_)
      );
    }
  }

 private:
  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiAutoSetReadOnly *>(user_data);
    auto &cfg = PreviewConfig::instance();
    if (!DOC_VALID(doc) || !cfg.get<bool>("tweakui/auto_set_read_only", false)) {
      return;
    }

    if (doc->real_path && euidaccess(doc->real_path, W_OK) != 0) {
      self->setReadOnly();
    }
  }

  GtkCheckMenuItem *read_only_menu_item_ = nullptr;
};

template class TweakUiBase<TweakUiAutoSetReadOnly>;
