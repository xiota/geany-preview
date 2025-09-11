// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_menu.h"

#include <cstdlib>  // std::system
#include <filesystem>

#include <glib.h>  // g_message
#include <gtk/gtk.h>
#include <msgwindow.h>

#ifndef HAVE_PLUGINS
#  define HAVE_PLUGINS 1
#endif
#include <geany/pluginutils.h>

#include "preview_config.h"
#include "preview_context.h"
#include "webview.h"

PreviewMenu::PreviewMenu(PreviewContext *context) : context_(context) {
  // Create the "Preview" submenu root
  submenu_root_ = gtk_menu_item_new_with_label("Preview");
  GtkWidget *submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu_root_), submenu);

  // Build menu items from the static table
  for (const auto &def : menu_defs_) {
    if (!def.label) {
      // Separator row
      gtk_menu_shell_append(GTK_MENU_SHELL(submenu), gtk_separator_menu_item_new());
      continue;
    }
    GtkWidget *item = gtk_menu_item_new_with_label(def.label);
    if (def.tooltip) {
      gtk_widget_set_tooltip_text(item, def.tooltip);
    }
    g_signal_connect(item, "activate", def.callback, context_);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
  }

  // Attach submenu to Geany's Tools menu
  GtkWidget *tools_menu = context_->geany_data_->main_widgets->tools_menu;
  gtk_menu_shell_append(GTK_MENU_SHELL(tools_menu), submenu_root_);

  gtk_widget_show_all(submenu_root_);
}

// --- Callbacks ---

void PreviewMenu::onExportToHtml(GtkMenuItem *, gpointer user_data) {
  auto *ctx = static_cast<PreviewContext *>(user_data);
  if (!ctx || !ctx->webview_) {
    return;
  }

  // TODO: Implement actual export logic
  // For now, just log to the message window
  msgwin_status_add("Preview: Export to HTML triggered");
}

void PreviewMenu::onPrint(GtkMenuItem *, gpointer user_data) {
  auto *ctx = static_cast<PreviewContext *>(user_data);
  if (ctx && ctx->webview_) {
    ctx->webview_->print(GTK_WINDOW(ctx->geany_data_->main_widgets->window));
  }
}

void PreviewMenu::onOpenConfigFolder(GtkMenuItem *, gpointer user_data) {
  auto *ctx = static_cast<PreviewContext *>(user_data);
  if (!ctx || !ctx->preview_config_) {
    return;
  }

  const auto &path = ctx->preview_config_->configDir();
  if (!std::filesystem::exists(path)) {
    msgwin_status_add("Preview: Config folder does not exist.");
    return;
  }

  std::string cmd = "xdg-open \"" + path.string() + "\"";
  std::ignore = std::system(cmd.c_str());
}

void PreviewMenu::onPreferences(GtkMenuItem *, gpointer user_data) {
  auto *ctx = static_cast<PreviewContext *>(user_data);
  if (!ctx || !ctx->geany_plugin_) {
    return;
  }

  plugin_show_configure(ctx->geany_plugin_);
}
