// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <geanyplugin.h>

#include "preview_pane.h"

namespace {
PreviewPane *preview_pane;

gboolean onEditorNotify(
    GObject * /*object*/,
    GeanyEditor *editor,
    SCNotification *notification,
    gpointer /*user_data*/
) {
  if (!notification || notification->nmhdr.code != SCN_MODIFIED) {
    return FALSE;
  }

  GeanyDocument *geany_document = editor ? editor->document : document_get_current();

  preview_pane->update(geany_document);
  return FALSE;
}

void onDocumentActivate(
    GObject * /*object*/,
    GeanyDocument *geany_document,
    gpointer /*user_data*/
) {
  preview_pane->update(geany_document);
}

gboolean previewInit(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  preview_pane = new PreviewPane(plugin->geany_data->main_widgets->sidebar_notebook);
  preview_pane->update();

  plugin_signal_connect(
      plugin, nullptr, "editor-notify", FALSE, G_CALLBACK(onEditorNotify), nullptr
  );

  plugin_signal_connect(
      plugin, nullptr, "document-activate", FALSE, G_CALLBACK(onDocumentActivate), nullptr
  );

  return TRUE;
}

void previewCleanup(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  if (preview_pane) {
    delete preview_pane;
    preview_pane = nullptr;
  }
}
}  // namespace

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
