// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <memory>

#include <geanyplugin.h>

#include "preview_pane.h"
#include "preview_shortcuts.h"

namespace {
std::unique_ptr<PreviewPane> preview_pane;
std::unique_ptr<PreviewConfig> preview_config;
std::unique_ptr<PreviewShortcuts> preview_shortcuts;

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
  preview_config = std::make_unique<PreviewConfig>(preview_config_path);
  preview_config->load();

  preview_pane = std::make_unique<PreviewPane>(
      plugin->geany_data->main_widgets->sidebar_notebook, preview_config.get()
  );

  plugin_signal_connect(
      plugin, nullptr, "editor-notify", FALSE, G_CALLBACK(onEditorNotify), nullptr
  );

  plugin_signal_connect(
      plugin, nullptr, "document-activate", FALSE, G_CALLBACK(onDocumentActivate), nullptr
  );

  // shortcuts
  GeanyKeyGroup *group = plugin_set_key_group(plugin, "Preview", 1, nullptr);
  preview_shortcuts = std::make_unique<PreviewShortcuts>(group);
  preview_shortcuts->registerAll();

  return TRUE;
}

void previewCleanup(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  if (preview_config) {
    preview_config->save();
  }
  preview_pane.reset();
  preview_config.reset();
  preview_shortcuts.reset();
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
