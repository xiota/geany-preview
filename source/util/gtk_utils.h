// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>

namespace GtkUtils {

// Returns true if the widget can meaningfully receive keyboard focus.
bool canFocusForReal(GtkWidget *widget);

// Depth-first search for the first focusable descendant (or the widget itself).
GtkWidget *findFocusableChild(GtkWidget *parent);

// Walks up to find the first ancestor of the given GType (e.g., GTK_TYPE_NOTEBOOK).
GtkWidget *findAncestorOfType(GtkWidget *widget, GType type);

// Activates the notebook page containing `widget` (or the current page if `widget` is a
// notebook), then focuses the first usable widget within that page.
// Returns the widget that was focused, or nullptr.
GtkWidget *activateNotebookPageForWidget(GtkWidget *widget);

// Returns true if `widget` is on the currently visible page of `notebook`.
bool isWidgetOnVisibleNotebookPage(GtkNotebook *notebook, GtkWidget *widget);

}  // namespace GtkUtils
