// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"

#include <geanyplugin.h>

static PreviewPane preview_pane;

gboolean
onEditorNotify(GObject *obj, GeanyEditor *editor, SCNotification *nt, gpointer user_data) {
  if (!nt || nt->nmhdr.code != SCN_MODIFIED) {
    return FALSE;
  }

  GeanyDocument *gdoc = editor ? editor->document : document_get_current();

  preview_pane.update(gdoc);
  return FALSE;
}

void onDocumentActivate(GObject *obj, GeanyDocument *gdoc, gpointer user_data) {
  preview_pane.update(gdoc);
}

gboolean previewInit(GeanyPlugin *plugin, gpointer) {
  preview_pane.attachToParent(plugin->geany_data->main_widgets->sidebar_notebook)
      .selectAsCurrent();

  plugin_signal_connect(
      plugin, nullptr, "editor-notify", FALSE, G_CALLBACK(onEditorNotify), nullptr
  );

  plugin_signal_connect(
      plugin, nullptr, "document-activate", FALSE, G_CALLBACK(onDocumentActivate), nullptr
  );

  return TRUE;
}

void previewCleanup(GeanyPlugin *plugin, gpointer) {
  preview_pane.detachFromParent();
  while (gtk_events_pending()) {
    gtk_main_iteration_do(FALSE);
  }
}

extern "C" G_MODULE_EXPORT void geany_load_module(GeanyPlugin *plugin) {
  plugin->info->name = "Preview";
  plugin->info->description =
      "Preview pane for Markdown and other lightweight markup languages";
  plugin->info->version = "0.2";
  plugin->info->author = "xiota";

  plugin->funcs->init = previewInit;
  plugin->funcs->cleanup = previewCleanup;
  plugin->funcs->configure = nullptr;
  plugin->funcs->callbacks = nullptr;
  plugin->funcs->help = nullptr;

  GEANY_PLUGIN_REGISTER(plugin, 225);
}
