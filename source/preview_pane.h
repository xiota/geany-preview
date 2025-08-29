// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>
#include <gtk/gtk.h>

#include "gtk_attachable.h"
#include "webview.h"

class PreviewPane : public GtkAttachable<PreviewPane> {
 public:
  PreviewPane()
      : GtkAttachable("Preview"), container_(gtk_scrolled_window_new(nullptr, nullptr)) {
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(container_), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
    );

    webview_.LoadHtml("<h1>Hello from GTK3!</h1>");
    webview_.AttachToParent(container_);
  }

  GtkWidget *widget() const override {
    return container_;
  }

  void Update(GeanyDocument *doc);

 private:
  GtkWidget *container_;
  WebView webview_;
};
