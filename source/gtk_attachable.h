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
  ~GtkAttachable() {
    DetachFromParent();
    // Do not call ClearWeakPointer; GTK clears on widget destroy
  }

  // No copying/moving, unique ownership of underlying widget
  GtkAttachable(const GtkAttachable &) = delete;
  GtkAttachable &operator=(const GtkAttachable &) = delete;
  GtkAttachable(GtkAttachable &&) = delete;
  GtkAttachable &operator=(GtkAttachable &&) = delete;

  virtual GtkWidget *widget() const = 0;

  Derived &AttachToParent(GtkWidget *parent) {
    GtkWidget *w = widget();
    if (!parent || !w) {
      return static_cast<Derived &>(*this);
    }
    if (parent_ == parent) {
      return static_cast<Derived &>(*this);
    }
    if (parent_) {
      DetachFromParent();  // detach from old parent
    }

    // in case derived class forgot
    TrackWidget(w);

    if (GTK_IS_CONTAINER(parent)) {
      gtk_container_add(GTK_CONTAINER(parent), w);
      parent_ = parent;
      gtk_widget_show(w);
      ApplyContainerLabel();
    }

    return static_cast<Derived &>(*this);
  }

  // Detaches the widget from its current parent, if any.
  Derived &DetachFromParent() {
    GtkWidget *w = weak_widget_;
    if (parent_ && w) {
      if (!gtk_widget_in_destruction(w) && GTK_IS_CONTAINER(parent_)) {
        gtk_container_remove(GTK_CONTAINER(parent_), w);
      }
      parent_ = nullptr;
    }
    return static_cast<Derived &>(*this);
  }

  Derived &SelectAsCurrent() {
    GtkWidget *w = weak_widget_;
    if (!parent_ || !w) {
      return static_cast<Derived &>(*this);
    }

    if (GTK_IS_NOTEBOOK(parent_)) {
      auto page_num = gtk_notebook_page_num(GTK_NOTEBOOK(parent_), w);
      if (page_num != -1) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(parent_), page_num);
      }
    } else if (GTK_IS_STACK(parent_)) {
      gchar *name = nullptr;
      gtk_container_child_get(GTK_CONTAINER(parent_), w, "name", &name, NULL);
      if (name) {
        gtk_stack_set_visible_child_name(GTK_STACK(parent_), name);
        g_free(name);
      }
    } else if (GTK_IS_ASSISTANT(parent_)) {
      for (int i = 0, n = gtk_assistant_get_n_pages(GTK_ASSISTANT(parent_)); i < n; ++i) {
        if (gtk_assistant_get_nth_page(GTK_ASSISTANT(parent_), i) == w) {
          gtk_assistant_set_current_page(GTK_ASSISTANT(parent_), i);
          break;
        }
      }
    } else if (GTK_IS_EXPANDER(parent_)) {
      gtk_expander_set_expanded(GTK_EXPANDER(parent_), TRUE);
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

  // TODO: connect to GtkWidget::parent-set signal to sync parent_
  GtkWidget *parent_ = nullptr;
  GtkWidget *weak_widget_ = nullptr;
  std::string label_;

  void ApplyContainerLabel() {
    GtkWidget *w = weak_widget_;
    if (label_.empty() || !parent_ || !w) {
      return;
    }

    if (GTK_IS_NOTEBOOK(parent_)) {
      gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(parent_), w, label_.c_str());
    } else if (GTK_IS_STACK(parent_)) {
      gtk_container_child_set(GTK_CONTAINER(parent_), w, "name", label_.c_str(), NULL);
    } else if (GTK_IS_ASSISTANT(parent_)) {
      gtk_assistant_set_page_title(GTK_ASSISTANT(parent_), w, label_.c_str());
    } else if (GTK_IS_EXPANDER(parent_)) {
      gtk_expander_set_label(GTK_EXPANDER(parent_), label_.c_str());
    }
  }

  // Can be called multiple times.
  // Last call is kept in weak_widget_
  void TrackWidget(GtkWidget *w) {
    if (!w) {
      return;
    }
    if (weak_widget_ == w) {
      return;
    }

    // Intentionally overwrites weak_widget_ without removing the old weak pointer
    weak_widget_ = w;
    g_object_add_weak_pointer(
        G_OBJECT(weak_widget_), reinterpret_cast<gpointer *>(&weak_widget_)
    );
  }

  void ClearWeakPointer(GtkWidget *w = nullptr) {
    if ((!w && weak_widget_) || (w && w == weak_widget_)) {
      // Remove weak pointer for the tracked widget
      g_object_remove_weak_pointer(
          G_OBJECT(weak_widget_), reinterpret_cast<gpointer *>(&weak_widget_)
      );
      weak_widget_ = nullptr;
    } else if (w) {
      // Remove weak pointer for a different widget
      g_object_remove_weak_pointer(G_OBJECT(w), reinterpret_cast<gpointer *>(&w));
    }
  }
};
