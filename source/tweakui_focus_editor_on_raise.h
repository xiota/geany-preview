// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <keybindings.h>  // for keybindings_send_command
#include <msgwindow.h>
#include <pluginutils.h>

#include "preview_config.h"
#include "preview_context.h"

class TweakUiFocusEditorOnRaise {
 public:
  explicit TweakUiFocusEditorOnRaise(PreviewContext *context) : context_(context) {
    if (!context_ || !context_->geany_data_ || !context_->geany_data_->main_widgets) {
      return;
    }

    GtkWidget *geany_window = context_->geany_data_->main_widgets->window;
    if (!geany_window) {
      return;
    }

    g_signal_connect(geany_window, "notify::is-active", G_CALLBACK(onWindowActive), this);
  }

 private:
  static void onWindowActive(GObject *object, GParamSpec *, gpointer user_data) {
    auto *self = static_cast<TweakUiFocusEditorOnRaise *>(user_data);

    if (!self->context_ || !self->context_->preview_config_) {
      return;
    }

    if (!self->context_->preview_config_->get<bool>("focus_editor_on_raise", false)) {
      return;
    }

    GtkWindow *window = GTK_WINDOW(object);
    if (gtk_window_is_active(window)) {
      g_timeout_add(
          100,
          [](gpointer) -> gboolean {
            keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
            return G_SOURCE_REMOVE;
          },
          nullptr
      );
    }
  }

  PreviewContext *context_ = nullptr;
};
