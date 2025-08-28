// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <string>
#include <string_view>

// Base class for self-contained GTK UI components with explicit
// attach/detach lifecycle control.
template <typename Derived>
class GtkAttachable {
 public:
  explicit GtkAttachable(std::string_view label) : label_(label) {}

  virtual ~GtkAttachable() {
    // Ensure we don't leave our widget attached somewhere
    DetachFromParent();
  }

  // The underlying GTK widget owned by this component
  virtual GtkWidget *widget() const = 0;

  // Attaches the widget to a parent container if not already attached.
  // If already attached to a different parent, you must detach first.
  Derived &AttachToParent(GtkContainer *parent) {
    if (!parent || !widget()) {
      return static_cast<Derived &>(*this);
    }
    if (parent_ == parent) {
      return static_cast<Derived &>(*this);
    }
    if (parent_) {
      DetachFromParent();  // detach from old parent
    }

    gtk_container_add(parent, widget());
    parent_ = parent;
    gtk_widget_show(widget());

    ApplyContainerLabel();
    return static_cast<Derived &>(*this);
  }

  // Detaches the widget from its current parent, if any.
  Derived &DetachFromParent() {
    if (parent_ && widget()) {
      gtk_container_remove(parent_, widget());
      parent_ = nullptr;
    }
    return static_cast<Derived &>(*this);
  }

  Derived &SetLabel(std::string_view label) {
    label_.assign(label);
    ApplyContainerLabel();
    return static_cast<Derived &>(*this);
  }

 protected:
  GtkAttachable() = default;

  GtkContainer *parent_ = nullptr;
  std::string label_;

  void ApplyContainerLabel() {
    if (label_.empty() || !parent_ || !widget()) {
      return;
    }

    if (GTK_IS_NOTEBOOK(parent_)) {
      gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(parent_), widget(), label_.c_str());

    } else if (GTK_IS_STACK(parent_)) {
      gtk_container_child_set(GTK_CONTAINER(parent_), widget(), "name", label_.c_str(), NULL);
    } else if (GTK_IS_ASSISTANT(parent_)) {
      gtk_assistant_set_page_title(GTK_ASSISTANT(parent_), widget(), label_.c_str());

    } else if (GTK_IS_EXPANDER(parent_)) {
      gtk_expander_set_label(GTK_EXPANDER(parent_), label_.c_str());
    }
  }
};
