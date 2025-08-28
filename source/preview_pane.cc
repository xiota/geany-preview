// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"

PreviewPane::PreviewPane() {
  // Create the main container for this pane
  container_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Placeholder content – replace with real preview rendering later
  GtkWidget *placeholder = gtk_label_new("No preview available");
  gtk_box_pack_start(GTK_BOX(container_), placeholder, TRUE, TRUE, 0);

  gtk_widget_show(container_);
}

PreviewPane::~PreviewPane() {
  detach_from_sidebar();
  container_ = nullptr;
}

GtkWidget *PreviewPane::widget() const {
  return container_;
}

void PreviewPane::update(GeanyDocument *doc) {
  if (!doc) {
    return;
  }

  // TODO: implement actual preview rendering logic here
}

void PreviewPane::attach_to_sidebar(GtkNotebook *sidebar) {
  if (!sidebar || !container_) {
    return;
  }
  GtkWidget *label = gtk_label_new("Preview");

  sidebar_ = sidebar;
  page_index_ = gtk_notebook_append_page(sidebar_, container_, label);
  GtkWidget *preview = gtk_notebook_get_nth_page(sidebar_, page_index_);
  gtk_widget_show(preview);
}

void PreviewPane::detach_from_sidebar() {
  if (sidebar_ && page_index_ >= 0) {
    gtk_notebook_remove_page(sidebar_, page_index_);
    page_index_ = -1;
    sidebar_ = nullptr;
  }
}
