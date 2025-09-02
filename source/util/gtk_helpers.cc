// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtk_helpers.h"

#include <gtk/gtk.h>

namespace GtkUtils {

bool canFocusForReal(GtkWidget *widget) {
  return widget && gtk_widget_get_can_focus(widget) && gtk_widget_get_visible(widget) &&
         gtk_widget_is_sensitive(widget);
}

GtkWidget *findFocusableChild(GtkWidget *parent) {
  if (canFocusForReal(parent)) {
    return parent;
  }

  if (GTK_IS_CONTAINER(parent)) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
    GtkWidget *result = nullptr;

    for (GList *l = children; l != nullptr; l = l->next) {
      GtkWidget *child = GTK_WIDGET(l->data);
      result = findFocusableChild(child);
      if (result) {
        break;
      }
    }

    g_list_free(children);
    return result;
  }

  return nullptr;
}

GtkWidget *findAncestorOfType(GtkWidget *widget, GType type) {
  while (widget && !g_type_is_a(G_OBJECT_TYPE(widget), type)) {
    widget = gtk_widget_get_parent(widget);
  }
  return widget;
}

GtkWidget *activateNotebookPageForWidget(GtkWidget *widget) {
  if (!widget) {
    return nullptr;
  }

  GtkNotebook *notebook = nullptr;
  GtkWidget *page = nullptr;

  // Support both calling with a notebook or with a descendant.
  if (GTK_IS_NOTEBOOK(widget)) {
    notebook = GTK_NOTEBOOK(widget);
    const int idx = gtk_notebook_get_current_page(notebook);
    page = gtk_notebook_get_nth_page(notebook, idx);
  } else {
    notebook = GTK_NOTEBOOK(findAncestorOfType(widget, GTK_TYPE_NOTEBOOK));
    if (!notebook) {
      return nullptr;
    }

    // Climb until we hit the page that was added to the notebook.
    page = widget;
    while (page && gtk_notebook_page_num(notebook, page) == -1) {
      page = gtk_widget_get_parent(page);
    }
  }

  if (!page) {
    return nullptr;
  }

  // Activate the page.
  gtk_notebook_set_current_page(notebook, gtk_notebook_page_num(notebook, page));

  // Choose the best focus target:
  // - If we were passed a descendant, prefer it when it’s focusable.
  // - Otherwise, find the first focusable descendant in the page.
  GtkWidget *target = nullptr;

  if (!GTK_IS_NOTEBOOK(widget) && canFocusForReal(widget)) {
    target = widget;
  }
  if (!target) {
    // If `widget` is a descendant, search its subtree first; fall back to the whole page.
    target = findFocusableChild(!GTK_IS_NOTEBOOK(widget) ? widget : page);
    if (!target) {
      target = findFocusableChild(page);
    }
  }

  if (target) {
    gtk_widget_grab_focus(target);
  }

  return target;
}

}  // namespace GtkUtils
