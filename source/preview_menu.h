// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iterator>

#include <gtk/gtk.h>

#include "preview_context.h"

class PreviewMenu {
 public:
  explicit PreviewMenu(PreviewContext *context);

  static constexpr gsize menuItemCount() {
    return std::size(menu_defs_);
  }

 private:
  PreviewContext *context_;
  GtkWidget *submenu_root_ = nullptr;

  static void onExportToHtml(GtkMenuItem *, gpointer user_data);
  static void onExportToPdf(GtkMenuItem *, gpointer user_data);
  static void onPrint(GtkMenuItem *, gpointer user_data);
  static void onOpenConfigFolder(GtkMenuItem *, gpointer user_data);
  static void onPreferences(GtkMenuItem *, gpointer user_data);

  struct MenuItemDef {
    const char *label;  // nullptr => separator
    const char *tooltip;
    GCallback callback;
  };

  // clang-format off
  inline static const MenuItemDef menu_defs_[] = {
    { "Export to HTML", "Export the current preview to an HTML file", G_CALLBACK(onExportToHtml) },
    { "Export to PDF", "Export the current preview to a PDF file", G_CALLBACK(onExportToPdf) },
    { nullptr, nullptr, nullptr }, // separator
    { "Print", "Print the current preview", G_CALLBACK(onPrint) },
    { nullptr, nullptr, nullptr }, // separator
    { "Open Config Folder", "Open the preview plugin's configuration folder", G_CALLBACK(onOpenConfigFolder) },
    { nullptr, nullptr, nullptr }, // separator
    { "Preferences", "Open the preview plugin preferences", G_CALLBACK(onPreferences) }
  };
  // clang-format on
};
