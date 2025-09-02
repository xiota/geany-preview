// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_shortcuts.h"

#include <cstddef>  // std::size

#include <gtk/gtk.h>      // gtk_widget_grab_focus()
#include <keybindings.h>  // Geany keybinding API

#include "preview_pane.h"

namespace {
PreviewContext *preview_context = nullptr;
}  // namespace

void PreviewShortcuts::onFocusPreview(guint /*key_id*/) {
  if (!preview_context) {
    return;
  }
  if (auto *widget =
          preview_context->preview_pane_ ? preview_context->preview_pane_->widget() : nullptr) {
    gtk_widget_grab_focus(widget);
  }
}

void PreviewShortcuts::onToggleFocus(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_) {
    return;
  }

  GtkWidget *preview = preview_context->preview_pane_->widget();
  if (!preview) {
    return;
  }

  if (gtk_widget_has_focus(preview)) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
  } else {
    gtk_widget_grab_focus(preview);
  }
}

void PreviewShortcuts::registerAll() {
  // Stash context for callbacks
  preview_context = context_;

  for (gsize i = 0; i < std::size(shortcut_defs_); ++i) {
    keybindings_set_item(
        group_,
        i,
        shortcut_defs_[i].callback,
        0,  // no default key
        static_cast<GdkModifierType>(0),
        shortcut_defs_[i].label,
        shortcut_defs_[i].tooltip,
        nullptr /* user_data unused here */
    );
  }
}
