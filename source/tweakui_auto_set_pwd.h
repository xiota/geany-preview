// SPDX-FileCopyrightText: Copyright 2025 xiota
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

class TweakUiAutoSetPwd {
 public:
  explicit TweakUiAutoSetPwd(PreviewContext *context) : context_(context) {
    if (!context_) {
      return;
    }

    context_->preview_config_->connectChanged([this]() {
      documentSignal(nullptr, nullptr, this);
    });

    // Hook into document activation (fires on open/new/switch)
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-activate",
        true,
        G_CALLBACK(documentSignal),
        this
    );
  }

 private:
  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiAutoSetPwd *>(user_data);
    if (!self->context_ || !self->context_->preview_config_) {
      return;
    }
    if (!self->context_->preview_config_->get<bool>("auto_set_pwd", false)) {
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

  PreviewContext *context_ = nullptr;
};
