// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdlib.h>  // setenv
#include <string>
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
    if (!DOC_VALID(doc) || !self->context_ || !self->context_->preview_config_) {
      return;
    }
    if (!self->context_->preview_config_->get<bool>("auto_set_pwd", false)) {
      return;
    }

    if (doc->real_path) {
      self->updatePwd(doc->real_path);
    }
  }

  void updatePwd(const char *path) {
    if (!path) {
      return;
    }

    std::string dir = g_path_get_dirname(path);
    // Walk up until we find an existing directory
    while (!dir.empty() && access(dir.c_str(), F_OK) != 0) {
      auto pos = dir.find_last_of('/');
      if (pos == std::string::npos) {
        dir.clear();
      } else {
        dir = dir.substr(0, pos);
      }
    }

    if (!dir.empty()) {
      if (chdir(dir.c_str()) == 0) {
        setenv("PWD", dir.c_str(), 1);
      }
    }
  }

  PreviewContext *context_ = nullptr;
};
