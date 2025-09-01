// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>

#include <geanyplugin.h>

#include "preview_pane.h"

namespace {
PreviewPane *preview_pane;
PreviewConfig *preview_config;

gboolean onEditorNotify(
    GObject * /*object*/,
    GeanyEditor *editor,
    SCNotification *notification,
    gpointer /*user_data*/
) {
  if (!notification || notification->nmhdr.code != SCN_MODIFIED) {
    return FALSE;
  }

  preview_pane->scheduleUpdate();
  return FALSE;
}

void onDocumentActivate(
    GObject * /*object*/,
    GeanyDocument *geany_document,
    gpointer /*user_data*/
) {
  preview_pane->scheduleUpdate();
}

GtkWidget *previewConfigure(
    GeanyPlugin * /*plugin*/,
    GtkDialog *dialog,
    gpointer /*user_data*/
) {
  return preview_config->buildConfigWidget(dialog);
}

gboolean previewInit(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  auto preview_config_path = std::filesystem::path(plugin->geany_data->app->configdir) /
                             "plugins" / "preview" / "preview.conf";
  preview_config = new PreviewConfig(preview_config_path);
  preview_config->load();

  preview_pane =
      new PreviewPane(plugin->geany_data->main_widgets->sidebar_notebook, preview_config);

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
  if (preview_config) {
    preview_config->save();

    delete preview_config;
    preview_config = nullptr;
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
  plugin->funcs->configure = previewConfigure;
  plugin->funcs->callbacks = nullptr;
  plugin->funcs->help = nullptr;

  GEANY_PLUGIN_REGISTER(plugin, 225);
}
