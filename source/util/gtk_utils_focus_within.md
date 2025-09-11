# GtkUtils — enableFocusWithinTracking

A GTK 3 utility in the **GtkUtils** namespace that simulates CSS `:focus-within` for any container widget.  
When any descendant of the tracked container gains keyboard focus, the container automatically gets a `focus-within` CSS class.  
When focus leaves entirely, the class is removed.

## Overview

GTK 3’s CSS engine does **not** support the `:focus-within` pseudo-class found in modern web CSS.  
That means you can’t style a container based on whether any of its children has focus — only the widget with focus itself.

`GtkUtils::enableFocusWithinTracking()` fills that gap by:
- Recursively hooking into all focusable descendants.
- Tracking dynamically added children.
- Adding/removing a `.focus-within` class on the container.

This lets you style the container as if GTK 3 supported `:focus-within` natively.

## Function Signature

```cpp
void GtkUtils::enableFocusWithinTracking(GtkWidget* page);
```

**Parameters:**
- `page` — The container widget you want to track (e.g. a `GtkNotebook` page, `GtkBox`, `GtkGrid`, etc.).

## Behavior

- **Initial scan**: Recursively connects `focus-in-event` and `focus-out-event` handlers to all existing focusable descendants.
- **Dynamic updates**: Listens for `"add"` signals to track newly added children automatically.
- **Class toggling**: Adds `.focus-within` to the container when any descendant gains focus; removes it when focus leaves the container entirely.
- **No flicker**: Moving focus between children inside the same container does not remove the class.

## Example Usage

```cpp
#include "gtk_utils.h"

GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
GtkUtils::enableFocusWithinTracking(page);

GtkWidget* entry = gtk_entry_new();
GtkWidget* button = gtk_button_new_with_label("Click me");
gtk_box_pack_start(GTK_BOX(page), entry, false, false, 0);
gtk_box_pack_start(GTK_BOX(page), button, false, false, 0);
```

**CSS:**
```css
.focus-within {
    background-color: #eef6ff;
    border: 1px solid #4a90e2;
    padding: 8px;
}
```

## When to Use

- Highlighting the active section of a form when any field inside it is focused.
- Styling the visible page of a `GtkNotebook` when the user is interacting with it.
- Providing visual cues for keyboard navigation.
