// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gtk_utils.h"

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

bool isWidgetOnVisibleNotebookPage(GtkNotebook *notebook, GtkWidget *widget) {
  if (!GTK_IS_NOTEBOOK(notebook) || !GTK_IS_WIDGET(widget)) {
    return false;
  }

  // Find the page number for this widget
  int page_num = gtk_notebook_page_num(notebook, widget);
  if (page_num < 0) {
    return false;  // widget not found in this notebook
  }

  // Compare with the currently visible page
  return gtk_notebook_get_current_page(notebook) == page_num;
}

namespace {  // for enableFocusWithinTracking

void addFocusWithin(GtkWidget *page) {
  GtkStyleContext *ctx = gtk_widget_get_style_context(page);
  if (!gtk_style_context_has_class(ctx, "focus-within")) {
    gtk_style_context_add_class(ctx, "focus-within");
  }
}

void removeFocusWithin(GtkWidget *page) {
  GtkStyleContext *ctx = gtk_widget_get_style_context(page);
  gtk_style_context_remove_class(ctx, "focus-within");
}

gboolean onFocusIn(GtkWidget *child, GdkEventFocus * /*event*/, gpointer page) {
  addFocusWithin(static_cast<GtkWidget *>(page));
  return false;  // propagate
}

gboolean onFocusOut(GtkWidget *child, GdkEventFocus * /*event*/, gpointer page) {
  GtkWidget *toplevel = gtk_widget_get_toplevel(static_cast<GtkWidget *>(page));
  if (GTK_IS_WINDOW(toplevel)) {
    GtkWidget *focusWidget = gtk_window_get_focus(GTK_WINDOW(toplevel));
    if (!focusWidget || !gtk_widget_is_ancestor(focusWidget, static_cast<GtkWidget *>(page))) {
      removeFocusWithin(static_cast<GtkWidget *>(page));
    }
  }
  return false;
}

void trackFocusForDescendants(GtkWidget *widget, GtkWidget *page) {
  if (gtk_widget_get_can_focus(widget)) {
    g_signal_connect(widget, "focus-in-event", G_CALLBACK(onFocusIn), page);
    g_signal_connect(widget, "focus-out-event", G_CALLBACK(onFocusOut), page);
  }

  if (GTK_IS_CONTAINER(widget)) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
    for (GList *l = children; l != nullptr; l = l->next) {
      trackFocusForDescendants(static_cast<GtkWidget *>(l->data), page);
    }
    g_list_free(children);
  }
}

void onChildAdded(GtkContainer * /*container*/, GtkWidget *child, gpointer page) {
  trackFocusForDescendants(child, static_cast<GtkWidget *>(page));
}

}  // namespace

void enableFocusWithinTracking(GtkWidget *page) {
  trackFocusForDescendants(page, page);
  g_signal_connect(page, "add", G_CALLBACK(onChildAdded), page);
}

bool hasFocusWithin(GtkWidget *widget) {
  if (!GTK_IS_WIDGET(widget)) {
    return false;
  }

  GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
  if (!GTK_IS_WINDOW(toplevel)) {
    return false;
  }

  GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(toplevel));
  return focus && gtk_widget_is_ancestor(focus, widget);
}

}  // namespace GtkUtils
