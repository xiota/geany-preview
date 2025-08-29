// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"
#include <geanyplugin.h>

static PreviewPane preview_pane;

static void on_document_activate(GObject *, GeanyDocument *doc, gpointer) {
  preview_pane.Update(doc);
}

gboolean preview_init(GeanyPlugin *plugin, gpointer) {
  preview_pane.AttachToParent(plugin->geany_data->main_widgets->sidebar_notebook)
      .SelectAsCurrent();

  plugin_signal_connect(
      plugin, nullptr, "document-activate", TRUE, G_CALLBACK(on_document_activate), nullptr
  );

  return TRUE;
}

void preview_cleanup(GeanyPlugin *plugin, gpointer) {
  preview_pane.DetachFromParent();
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

  plugin->funcs->init = preview_init;
  plugin->funcs->cleanup = preview_cleanup;
  plugin->funcs->configure = nullptr;
  plugin->funcs->callbacks = nullptr;
  plugin->funcs->help = nullptr;

  GEANY_PLUGIN_REGISTER(plugin, 225);
}
