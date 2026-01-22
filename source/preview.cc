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
std::unique_ptr<PreviewPane> preview_pane;
std::unique_ptr<PreviewMenu> preview_menu;
std::unique_ptr<PreviewShortcuts> preview_shortcuts;

gboolean onEditorNotify(
    GObject * /*object*/,
    GeanyEditor *editor,
    SCNotification *notification,
    gpointer /*user_data*/
) {
  if (!notification || notification->nmhdr.code != SCN_MODIFIED) {
    return false;
  }

  preview_pane->scheduleUpdate();
  return false;
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
  auto &cfg = PreviewConfig::instance();
  return cfg.buildConfigWidget(dialog);
}

gboolean previewInit(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  // config
  auto config_path =
      std::filesystem::path(plugin->geany_data->app->configdir) / "plugins" / "preview";

  PreviewConfig::init(config_path, "preview.conf").load();

  // context
  auto &ctx = PreviewContext::instance();
  ctx.geany_plugin_ = plugin;
  ctx.geany_data_ = plugin->geany_data;
  ctx.geany_sidebar_ = plugin->geany_data->main_widgets->sidebar_notebook;

  // preview pane
  preview_pane = std::make_unique<PreviewPane>();
  ctx.preview_pane_ = preview_pane.get();

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
  preview_menu = std::make_unique<PreviewMenu>();

  // shortcuts
  ctx.geany_key_group_ =
      plugin_set_key_group(plugin, "Preview", PreviewShortcuts::shortcutCount(), nullptr);
  preview_shortcuts = std::make_unique<PreviewShortcuts>();

  return true;
}

void previewCleanup(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  PreviewConfig::instance().save();

  preview_pane.reset();
  preview_menu.reset();
  preview_shortcuts.reset();
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
