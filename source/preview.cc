// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <memory>

#include <geanyplugin.h>

#include "preview_config.h"
#include "preview_context.h"
#include "preview_pane.h"
#include "preview_shortcuts.h"
#include "tweakui_auto_set_read_only.h"
#include "tweakui_color_tip.h"
#include "tweakui_column_markers.h"
#include "tweakui_mark_word.h"
#include "tweakui_redetect_filetype.h"
#include "tweakui_sidebar_auto_resize.h"
#include "tweakui_unchange_document.h"

namespace {
std::unique_ptr<PreviewPane> preview_pane;
std::unique_ptr<PreviewConfig> preview_config;
std::unique_ptr<PreviewShortcuts> preview_shortcuts;

std::unique_ptr<TweakUiAutoSetReadOnly> tweakui_auto_set_read_only;
std::unique_ptr<TweakUiColorTip> tweakui_color_tip;
std::unique_ptr<TweakUiColumnMarkers> tweakui_column_markers;
std::unique_ptr<TweakUiMarkWord> tweakui_mark_word;
std::unique_ptr<TweakUiRedetectFileType> tweakui_redetect_filetype;
std::unique_ptr<TweakUiSidebarAutoResize> tweakui_sidebar_auto_resize;
std::unique_ptr<TweakUiUnchangeDocument> tweakui_unchange_document;

PreviewContext preview_context;

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
  return preview_config->buildConfigWidget(dialog);
}

gboolean previewInit(
    GeanyPlugin *plugin,
    gpointer /*user_data*/
) {
  // config
  auto preview_config_path =
      std::filesystem::path(plugin->geany_data->app->configdir) / "plugins" / "preview";
  preview_config = std::make_unique<PreviewConfig>(preview_config_path, "preview.conf");
  preview_config->load();

  // context
  preview_context.geany_plugin_ = plugin;
  preview_context.geany_data_ = plugin->geany_data;
  preview_context.geany_sidebar_ = plugin->geany_data->main_widgets->sidebar_notebook;
  preview_context.preview_config_ = preview_config.get();

  // preview pane
  preview_pane = std::make_unique<PreviewPane>(&preview_context);
  preview_context.preview_pane_ = preview_pane.get();

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
  tweakui_auto_set_read_only = std::make_unique<TweakUiAutoSetReadOnly>(&preview_context);
  tweakui_color_tip = std::make_unique<TweakUiColorTip>(&preview_context);
  tweakui_column_markers = std::make_unique<TweakUiColumnMarkers>(&preview_context);
  tweakui_mark_word = std::make_unique<TweakUiMarkWord>(&preview_context);
  tweakui_redetect_filetype = std::make_unique<TweakUiRedetectFileType>(&preview_context);
  tweakui_sidebar_auto_resize = std::make_unique<TweakUiSidebarAutoResize>(&preview_context);
  tweakui_unchange_document = std::make_unique<TweakUiUnchangeDocument>(&preview_context);

  // shortcuts
  preview_context.geany_key_group_ =
      plugin_set_key_group(plugin, "Preview", PreviewShortcuts::shortcutCount(), nullptr);
  preview_shortcuts = std::make_unique<PreviewShortcuts>(&preview_context);

  return true;
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

  tweakui_auto_set_read_only.reset();
  tweakui_color_tip.reset();
  tweakui_column_markers.reset();
  tweakui_mark_word.reset();
  tweakui_redetect_filetype.reset();
  tweakui_sidebar_auto_resize.reset();
  tweakui_unchange_document.reset();
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
