// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @brief Lightweight struct holding non-owning pointers to core plugin components.
 */

#include <gtk/gtk.h>
#include <plugindata.h>
#include <ui_utils.h>

class PreviewPane;
class PreviewConfig;
class WebView;

struct PreviewContext {
  GeanyPlugin *geany_plugin_ = nullptr;
  GeanyData *geany_data_ = nullptr;
  GtkWidget *geany_sidebar_ = nullptr;
  GeanyKeyGroup *geany_key_group_ = nullptr;

  PreviewPane *preview_pane_ = nullptr;
  PreviewConfig *preview_config_ = nullptr;
  WebView *webview_ = nullptr;
};
