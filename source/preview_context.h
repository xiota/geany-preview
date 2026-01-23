// SPDX-FileCopyrightText: Copyright 2025-2026 xiota
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

class PreviewContext {
 public:
  static PreviewContext &instance() {
    static PreviewContext inst;
    return inst;
  }

 private:
  PreviewContext() = default;
  ~PreviewContext() = default;

  PreviewContext(const PreviewContext &) = delete;
  PreviewContext &operator=(const PreviewContext &) = delete;
  PreviewContext(PreviewContext &&) = delete;
  PreviewContext &operator=(PreviewContext &&) = delete;

 public:
  GeanyPlugin *geany_plugin_ = nullptr;
  GeanyData *geany_data_ = nullptr;
  GtkWidget *geany_sidebar_ = nullptr;

  void openPreferences() const {
    if (geany_plugin_) {
      plugin_show_configure(geany_plugin_);
    }
  }
};
