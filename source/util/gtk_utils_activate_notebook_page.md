# GtkUtils — activateNotebookPageForWidget

Part of the **GtkUtils** toolkit: a GTK 3 utility that ensures the correct `GtkNotebook` page is active for a given widget, and focuses the most appropriate child inside it.

## Overview

`GtkUtils::activateNotebookPageForWidget()` is designed to simplify the common task of:

1. Determining which notebook page contains a given widget (or using the notebook directly).
2. Switching the notebook to that page if it’s not already active.
3. Selecting the best focus target inside that page so the user can immediately interact with it.

It handles both direct `GtkNotebook` pointers and arbitrary descendant widgets.

## Function Signature

```cpp
GtkWidget* GtkUtils::activateNotebookPageForWidget(GtkWidget* widget);
```

**Parameters:**
- `widget` — Either:
  - A `GtkNotebook` instance, or
  - Any widget contained within a notebook page.

**Returns:**
- The widget that was ultimately given focus, or `nullptr` if no suitable focus target was found.

## Behavior

- **Notebook detection**  
  If `widget` is a `GtkNotebook`, the function operates on its current page.  
  If `widget` is a descendant of a notebook, it climbs the widget hierarchy to find the containing notebook and the page widget.

- **Page activation**  
  If the page containing `widget` is not currently active, the function switches to it.

- **Focus target selection**  
  The function chooses the best widget to focus:
  1. If `widget` is not a notebook and is focusable (`canFocusForReal()`), it is focused directly.
  2. Otherwise, it searches for the first focusable descendant of `widget` (if `widget` is a descendant) or of the page.
  3. If no focusable widget is found in the subtree, it searches the entire page.

- **Focus grab**  
  If a focusable widget is found, `gtk_widget_grab_focus()` is called on it.

## Example Usage

```cpp
#include "gtk_utils.h"

// Example: Ensure the page containing `someChild` is active and focused
GtkWidget* focused = GtkUtils::activateNotebookPageForWidget(someChild);

if (focused) {
    g_print("Focused widget: %s\n", gtk_widget_get_name(focused));
} else {
    g_print("No focusable widget found.\n");
}
```

## When to Use

- **Navigating to a widget’s page** in a multi‑tabbed interface when the widget needs to be shown and focused.
- **Responding to search results**: If a search result corresponds to a widget inside a notebook, this function ensures the user sees it immediately.
- **Keyboard shortcuts**: Jumping to a specific tool or form field inside a notebook page.
