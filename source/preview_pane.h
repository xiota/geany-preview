// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>
#include <gtk/gtk.h>

#include "converter_registrar.h"
#include "gtk_attachable.h"
#include "webview.h"

class PreviewPane final : public GtkAttachable<PreviewPane> {
 public:
  PreviewPane()
      : GtkAttachable("Preview"), container_(gtk_scrolled_window_new(nullptr, nullptr)) {
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(container_), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
    );

    webview_.attachToParent(container_);
  }

  GtkWidget *widget() const override {
    return container_;
  }

  void update(GeanyDocument *document);

 private:
  GtkWidget *container_;
  WebView webview_;
  ConverterRegistrar registrar_;
};
