// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>
#include <gtk/gtk.h>

class PreviewPane {
 public:
  PreviewPane();
  ~PreviewPane();

  GtkWidget *widget() const;
  void update(GeanyDocument *doc);

  void attach_to_sidebar(GtkNotebook *sidebar);
  void detach_from_sidebar();

 private:
  GtkWidget *container_ = nullptr;
  GtkNotebook *sidebar_ = nullptr;
  int page_index_ = -1;
};
