// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iterator>

#include <gtk/gtk.h>

#include "preview_context.h"

class PreviewMenu {
 public:
  static PreviewMenu &instance() {
    static PreviewMenu inst;
    return inst;
  }

 private:
  PreviewMenu();
  ~PreviewMenu() = default;

  PreviewMenu(const PreviewMenu &) = delete;
  PreviewMenu &operator=(const PreviewMenu &) = delete;
  PreviewMenu(PreviewMenu &&) = delete;
  PreviewMenu &operator=(PreviewMenu &&) = delete;

 public:
  static constexpr gsize menuItemCount() {
    return std::size(menu_defs_);
  }

 private:
  GtkWidget *submenu_root_ = nullptr;

  static void onExportToHtml(GtkMenuItem *, gpointer user_data);
  static void onExportToPdf(GtkMenuItem *, gpointer user_data);
  static void onPrint(GtkMenuItem *, gpointer user_data);
  static void onOpenConfigFolder(GtkMenuItem *, gpointer user_data);
  static void onPreferences(GtkMenuItem *, gpointer user_data);

  using MenuCallback = void (*)(GtkMenuItem *, gpointer);
  struct MenuItemDef {
    const char *label;  // nullptr => separator
    const char *tooltip;
    MenuCallback callback;
  };

  // clang-format off
  inline static const MenuItemDef menu_defs_[] = {
    { "Export to HTML…",
      "Export the current preview to an HTML file",
      onExportToHtml },

    { "Export to PDF…",
      "Export the current preview to a PDF file",
      onExportToPdf },

    { nullptr, nullptr, nullptr }, // separator

    { "Print…",
      "Print the current preview",
      onPrint },

    { nullptr, nullptr, nullptr }, // separator

    { "Open Config Folder",
      "Open the preview plugin's configuration folder",
      onOpenConfigFolder },

    { nullptr, nullptr, nullptr }, // separator

    { "Preferences",
      "Open the preview plugin preferences",
      onPreferences }
  };
  // clang-format on
};
