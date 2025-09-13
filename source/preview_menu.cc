// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_menu.h"

#include <filesystem>

#include <gtk/gtk.h>
#include <msgwindow.h>

#ifndef HAVE_PLUGINS
#  define HAVE_PLUGINS 1
#endif
#include <geany/pluginutils.h>

#include "preview_config.h"
#include "preview_context.h"
#include "preview_pane.h"
#include "subprocess.h"
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
  if (!ctx || !ctx->preview_pane_) {
    return;
  }

  // Suggest a default filename based on current document
  Document doc(document_get_current());
  std::filesystem::path default_name = std::filesystem::path(doc.fileName()).filename();
  if (default_name.empty() || default_name == ".") {
    default_name = "preview.html";
  } else {
    default_name.replace_extension(".html");
  }

  GtkWindow *parent = GTK_WINDOW(ctx->geany_data_->main_widgets->window);
  GtkWidget *dlg = gtk_file_chooser_dialog_new(
      "Export Preview to HTML",
      parent,
      GTK_FILE_CHOOSER_ACTION_SAVE,
      "_Cancel",
      GTK_RESPONSE_CANCEL,
      "_Save",
      GTK_RESPONSE_ACCEPT,
      nullptr
  );
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), true);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), default_name.string().c_str());

  std::filesystem::path dest_path;
  if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    if (filename) {
      dest_path = std::filesystem::path(filename);
      g_free(filename);
    }
  }
  gtk_widget_destroy(dlg);

  if (dest_path.empty()) {
    return;  // user cancelled
  }

  if (ctx->preview_pane_->exportHtmlToFile(dest_path)) {
    msgwin_status_add("Preview: Exported HTML to: %s", dest_path.string().c_str());
  } else {
    msgwin_status_add("Preview: Failed to export HTML.");
  }
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

  if (!Subprocess::runAsync(cmd)) {
    msgwin_status_add("Preview: No suitable terminal found.");
    return;
  }
}

void PreviewMenu::onPreferences(GtkMenuItem *, gpointer user_data) {
  auto *ctx = static_cast<PreviewContext *>(user_data);
  if (!ctx || !ctx->geany_plugin_) {
    return;
  }

  plugin_show_configure(ctx->geany_plugin_);
}
