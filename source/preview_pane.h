// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>
#include <gtk/gtk.h>

#include "gtk_attachable.h"

class PreviewPane : public GtkAttachable<PreviewPane> {
 public:
  PreviewPane()
      : GtkAttachable("Preview"), container_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)) {}

  GtkWidget *widget() const override {
    return container_;
  }

  void Update(GeanyDocument *doc);

 private:
  GtkWidget *container_;
};
