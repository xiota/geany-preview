// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @brief Lightweight class holding non-owning pointers to core plugin components.
 */

#include <gtk/gtk.h>
#include <keybindings.h>
#include <plugindata.h>
#include <ui_utils.h>

#ifndef HAVE_PLUGINS
#  define HAVE_PLUGINS 1
#endif
#include <geany/pluginutils.h>

class PreviewPane;
class PreviewConfig;
class WebView;

class PreviewContext {
 public:
  GeanyPlugin *geany_plugin_ = nullptr;
  GeanyData *geany_data_ = nullptr;
  GtkWidget *geany_sidebar_ = nullptr;
  GeanyKeyGroup *geany_key_group_ = nullptr;

  PreviewPane *preview_pane_ = nullptr;
  PreviewConfig *preview_config_ = nullptr;
  WebView *webview_ = nullptr;

  void openPreferences() const {
    if (geany_plugin_) {
      plugin_show_configure(geany_plugin_);
    }
  }
};
