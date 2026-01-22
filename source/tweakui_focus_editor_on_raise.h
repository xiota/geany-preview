// SPDX-FileCopyrightText: Copyright 2025-2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <keybindings.h>  // for keybindings_send_command
#include <msgwindow.h>
#include <pluginutils.h>

#include "preview_config.h"
#include "preview_context.h"
#include "tweakui_base.h"

class TweakUiFocusEditorOnRaise : public TweakUiBase<TweakUiFocusEditorOnRaise> {
 public:
  explicit TweakUiFocusEditorOnRaise() {
    auto &ctx = PreviewContext::instance();
    if (!ctx.geany_data_ || !ctx.geany_data_->main_widgets) {
      return;
    }

    GtkWidget *geany_window = ctx.geany_data_->main_widgets->window;
    if (!geany_window) {
      return;
    }

    g_signal_connect(geany_window, "notify::is-active", G_CALLBACK(onWindowActive), nullptr);
  }

 private:
  static void onWindowActive(GObject *object, GParamSpec *, gpointer user_data) {
    auto &cfg = PreviewConfig::instance();
    if (!cfg.get<bool>("focus_editor_on_raise", false)) {
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
};

template class TweakUiBase<TweakUiFocusEditorOnRaise>;
