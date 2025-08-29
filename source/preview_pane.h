// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>
#include <gtk/gtk.h>

#include "gtk_attachable.h"

class PreviewPane : public GtkAttachable<PreviewPane> {
 public:
  PreviewPane()
      : GtkAttachable("Preview"), container_(gtk_scrolled_window_new(nullptr, nullptr)) {
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(container_), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
    );

    GtkWidget *label = gtk_label_new("Hello from GTK3!");
    gtk_container_add(GTK_CONTAINER(container_), label);
    gtk_widget_show(label);
  }

  GtkWidget *widget() const override {
    return container_;
  }

  void Update(GeanyDocument *doc);

 private:
  GtkWidget *container_;
};
