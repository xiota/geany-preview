# GtkUtils

A collection of small, reusable GTK 3 utility functions for widget focus handling, notebook navigation, and CSS styling hooks.

## Features

### Focus Utilities
- **`canFocusForReal(GtkWidget*)`**  
  Checks if a widget is focusable, visible, and sensitive.
- **`findFocusableChild(GtkWidget*)`**  
  Recursively finds the first focusable descendant of a widget.
- **`findAncestorOfType(GtkWidget*, GType)`**  
  Walks up the widget hierarchy to find the nearest ancestor of a given type.

### Notebook Helpers
- **`activateNotebookPageForWidget(GtkWidget*)`**  
  Ensures the notebook page containing the given widget is active, and focuses the best available target inside it.
- **`isWidgetOnVisibleNotebookPage(GtkNotebook*, GtkWidget*)`**  
  Checks if a widget is on the currently visible page of a notebook.

### CSS “focus-within” Tracking
- **`enableFocusWithinTracking(GtkWidget*)`**  
  Adds a `.focus-within` CSS class to a container whenever any descendant has focus, and removes it when focus leaves entirely.  
  Works for existing and dynamically added children.

## Usage Example
```cpp
#include "gtk_utils.h"

GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
GtkUtils::enableFocusWithinTracking(page);

GtkWidget* entry = gtk_entry_new();
GtkWidget* button = gtk_button_new_with_label("Click me");
gtk_box_pack_start(GTK_BOX(page), entry, FALSE, FALSE, 0);
gtk_box_pack_start(GTK_BOX(page), button, FALSE, FALSE, 0);
```

**CSS:**
```css
.focus-within {
    background-color: #eef6ff;
    border: 1px solid #4a90e2;
}
```
