// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_shortcuts.h"

#include <cstddef>  // std::size

#include <gtk/gtk.h>  // gtk_widget_grab_focus()
#include <keybindings.h>
#include <plugindata.h>
#include <ui_utils.h>

#include "preview_pane.h"
#include "util/gtk_helpers.h"

namespace {
PreviewContext *preview_context = nullptr;
}  // namespace

void PreviewShortcuts::onFocusPreview(guint /*key_id*/) {
  if (!preview_context) {
    return;
  }
  if (auto *widget =
          preview_context->preview_pane_ ? preview_context->preview_pane_->widget() : nullptr) {
    GtkUtils::activateNotebookPageForWidget(widget);
  }
}

void PreviewShortcuts::onFocusPreviewEditor(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;
  GtkWidget *preview = preview_context->preview_pane_->widget();

  const bool in_editor = sci && gtk_widget_has_focus(sci);
  const bool in_preview = preview && gtk_widget_has_focus(preview);

  if (!in_editor && !in_preview) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  if (in_editor) {
    if (preview) {
      GtkUtils::activateNotebookPageForWidget(preview);
    }
    return;
  }

  // in_preview
  keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
}

void PreviewShortcuts::onFocusSidebarEditor(guint /*key_id*/) {
  if (!preview_context || !preview_context->geany_plugin_) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;

  GtkWidget *sidebar = nullptr;
  if (preview_context->geany_plugin_->geany_data &&
      preview_context->geany_plugin_->geany_data->main_widgets) {
    sidebar = preview_context->geany_plugin_->geany_data->main_widgets->sidebar_notebook;
  }

  const bool in_editor = sci && gtk_widget_has_focus(sci);
  const bool in_sidebar = sidebar && gtk_widget_has_focus(sidebar);

  if (!in_editor && !in_sidebar) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  if (in_editor) {
    if (sidebar) {
      GtkUtils::activateNotebookPageForWidget(sidebar);
    }
    return;
  }

  // in_sidebar
  keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
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
