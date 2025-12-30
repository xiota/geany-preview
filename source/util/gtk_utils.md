# GtkUtils

A collection of small, reusable GTK 3 utility functions for widget focus handling, notebook navigation, and CSS styling hooks.

## Features

### Focus Utilities
- **`canFocusForReal(GtkWidget*)`**  
  Returns `true` if a widget is focusable, visible, and sensitive.
- **`findFocusableChild(GtkWidget*)`**  
  Recursively finds the first focusable descendant of a widget.
- **`findAncestorOfType(GtkWidget*, GType)`**  
  Walks up the widget hierarchy to find the nearest ancestor of a given type.
- **`hasFocusWithin(GtkWidget*)`**  
  Returns `true` if the widget itself or any of its descendants currently has keyboard focus.

### Notebook Helpers
- **`activateNotebookPageForWidget(GtkWidget*)`**  
  Ensures the notebook page containing the given widget is active, and focuses the best available target inside it.
- **`isWidgetOnVisibleNotebookPage(GtkNotebook*, GtkWidget*)`**  
  Returns `true` if a widget is on the currently visible page of a notebook.
