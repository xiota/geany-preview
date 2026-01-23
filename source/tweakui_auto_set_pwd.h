// SPDX-FileCopyrightText: Copyright 2025-2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>

#include <stdlib.h>  // setenv
#include <unistd.h>  // chdir

#include <glib.h>
#include <gtk/gtk.h>
#include <pluginutils.h>  // plugin_signal_connect()

#include "preview_config.h"
#include "preview_context.h"
#include "tweakui_base.h"

class TweakUiAutoSetPwd : public TweakUiBase<TweakUiAutoSetPwd> {
 public:
  explicit TweakUiAutoSetPwd() {
    auto &ctx = PreviewContext::instance();
    if (!ctx.geany_plugin_) {
      return;
    }

    PreviewConfig::registerSetting(
        "tweakui/auto_set_pwd",
        false,
        "Set $PWD to the current document’s folder, walking up if needed."
    );

    auto &cfg = PreviewConfig::instance();
    cfg.connectChanged([this]() { documentSignal(nullptr, nullptr, this); });

    // Hook into document activation (fires on open/new/switch)
    plugin_signal_connect(
        ctx.geany_plugin_, nullptr, "document-activate", true, G_CALLBACK(documentSignal), this
    );
  }

 private:
  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiAutoSetPwd *>(user_data);
    auto &cfg = PreviewConfig::instance();
    if (cfg.get<bool>("tweakui/auto_set_pwd", false)) {
      return;
    }

    if (!DOC_VALID(doc)) {
      doc = document_get_current();
      g_return_if_fail(DOC_VALID(doc));
    }

    if (doc->real_path) {
      self->updatePwd(doc->real_path);
    }
  }

  void updatePwd(const char *path) {
    if (!path) {
      return;
    }

    std::filesystem::path fsPath(path);
    std::filesystem::path dir = fsPath.parent_path();

    // Walk up until we find an existing directory
    while (!dir.empty() && !std::filesystem::exists(dir)) {
      dir = dir.parent_path();
    }

    if (!dir.empty()) {
      if (chdir(dir.c_str()) == 0) {
        setenv("PWD", dir.c_str(), 1);
      }
    }
  }
};

template class TweakUiBase<TweakUiAutoSetPwd>;
