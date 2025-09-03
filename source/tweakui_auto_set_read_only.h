// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
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

class TweakUiAutoSetReadOnly {
 public:
  explicit TweakUiAutoSetReadOnly(PreviewContext *context) : context_(context) {
    if (context_ && context_->geany_data_ && context_->geany_data_->main_widgets->window) {
      read_only_menu_item_ = GTK_CHECK_MENU_ITEM(ui_lookup_widget(
          GTK_WIDGET(context_->geany_data_->main_widgets->window), "set_file_readonly1"
      ));
    }

    plugin_signal_connect(
        context_->geany_plugin_,
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

    if (!DOC_VALID(doc) || !self->context_ || !self->context_->preview_config_ ||
        !self->context_->preview_config_->get<bool>("auto_set_read_only", false)) {
      return;
    }

    if (doc->real_path && euidaccess(doc->real_path, W_OK) != 0) {
      self->setReadOnly();
    }
  }

  PreviewContext *context_ = nullptr;
  GtkCheckMenuItem *read_only_menu_item_ = nullptr;
};
