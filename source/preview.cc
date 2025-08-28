// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"
#include <geanyplugin.h>

static PreviewPane *preview_pane = nullptr;

static void on_document_activate(GObject *, GeanyDocument *doc, gpointer) {
  if (preview_pane) {
    preview_pane->update(doc);
  }
}

gboolean preview_init(GeanyPlugin *plugin, gpointer) {
  preview_pane = new PreviewPane();
  preview_pane->attach_to_sidebar(
      GTK_NOTEBOOK(plugin->geany_data->main_widgets->sidebar_notebook)
  );

  plugin_signal_connect(
      plugin, nullptr, "document-activate", TRUE, G_CALLBACK(on_document_activate), nullptr
  );
  return TRUE;
}

void preview_cleanup(GeanyPlugin *plugin, gpointer) {
  if (preview_pane) {
    preview_pane->detach_from_sidebar();
    delete preview_pane;
    preview_pane = nullptr;
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
