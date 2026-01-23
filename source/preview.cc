// SPDX-FileCopyrightText: Copyright 2025-2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <memory>

#include <geanyplugin.h>

#include "config.h"
#include "preview_config.h"
#include "preview_context.h"
#include "preview_menu.h"
#include "preview_pane.h"
#include "preview_shortcuts.h"
#include "tweakui_auto_set_pwd.h"
#include "tweakui_auto_set_read_only.h"
#include "tweakui_color_tip.h"
#include "tweakui_column_markers.h"
#include "tweakui_disable_editor_ctrl_wheel_zoom.h"
#include "tweakui_focus_editor_on_raise.h"
#include "tweakui_mark_word.h"
#include "tweakui_redetect_filetype.h"
#include "tweakui_sidebar_auto_resize.h"
#include "tweakui_unchange_document.h"

namespace {
gboolean onEditorNotify(
    GObject * /*object*/,
    GeanyEditor *editor,
    SCNotification *notification,
    gpointer /*user_data*/
) {
  if (!notification || notification->nmhdr.code != SCN_MODIFIED) {
    return false;
  }

  auto &pane = PreviewPane::instance();
  pane.scheduleUpdate();
  return false;
}

void onDocumentActivate(
    GObject * /*object*/,
    GeanyDocument *geany_document,
    gpointer /*user_data*/
) {
  auto &pane = PreviewPane::instance();
  pane.scheduleUpdate();
}

GtkWidget *previewConfigure(
    GeanyPlugin * /*plugin*/,
    GtkDialog *dialog,
    gpointer /*user_data*/
) {
  auto &cfg = PreviewConfig::instance();
  return cfg.buildConfigWidget(dialog);
}

gboolean previewInit(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  // config - init
  auto config_path =
      std::filesystem::path(plugin->geany_data->app->configdir) / "plugins" / "preview";
  auto &cfg = PreviewConfig::init(config_path, "preview.conf");

  // context
  auto &ctx = PreviewContext::instance();
  ctx.geany_plugin_ = plugin;
  ctx.geany_data_ = plugin->geany_data;
  ctx.geany_sidebar_ = plugin->geany_data->main_widgets->sidebar_notebook;

  // preview pane
  PreviewPane::instance();

  // signals
  plugin_signal_connect(
      plugin, nullptr, "editor-notify", false, G_CALLBACK(onEditorNotify), nullptr
  );

  plugin_signal_connect(
      plugin, nullptr, "document-activate", false, G_CALLBACK(onDocumentActivate), nullptr
  );

  plugin_signal_connect(
      plugin, nullptr, "document-save", false, G_CALLBACK(onDocumentActivate), nullptr
  );

  // tweaks
  for (auto &tweakui_init : tweakui_registry()) {
    tweakui_init();
  }

  // menu
  PreviewMenu::instance();

  // shortcuts
  PreviewShortcuts::instance().init(plugin, plugin->info->name);

  // config - delayed load
  cfg.load();

  return true;
}

void previewCleanup(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  PreviewConfig::instance().save();
}
}  // namespace

extern "C" G_MODULE_EXPORT void geany_load_module(GeanyPlugin *plugin) {
  plugin->info->name = "Preview";
  plugin->info->description =
      "Preview pane for Markdown and other lightweight markup languages";
  plugin->info->version = VERSION;
  plugin->info->author = "xiota";

  plugin->funcs->init = previewInit;
  plugin->funcs->cleanup = previewCleanup;
  plugin->funcs->configure = previewConfigure;
  plugin->funcs->callbacks = nullptr;
  plugin->funcs->help = nullptr;

  GEANY_PLUGIN_REGISTER(plugin, 225);
}
