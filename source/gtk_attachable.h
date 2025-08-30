// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include <gtk/gtk.h>

// Base class for self-contained GTK UI components with explicit
// attach/detach lifecycle control.
template <typename Derived>
class GtkAttachable {
 public:
  explicit GtkAttachable(std::string_view label) : label_(label) {}
  ~GtkAttachable() {
    detachFromParent();
  }

  // No copying/moving, unique ownership of underlying widget
  GtkAttachable(const GtkAttachable &) = delete;
  GtkAttachable &operator=(const GtkAttachable &) = delete;
  GtkAttachable(GtkAttachable &&) = delete;
  GtkAttachable &operator=(GtkAttachable &&) = delete;

  virtual GtkWidget *widget() const = 0;

  Derived &attachToParent(GtkWidget *parent) {
    GtkWidget *w = widget();
    if (!parent || !w) {
      return static_cast<Derived &>(*this);
    }
    if (parent_ == parent) {
      return static_cast<Derived &>(*this);
    }
    if (parent_) {
      detachFromParent();  // detach from old parent
    }

    // in case derived class forgot
    trackWidget(w);

    if (GTK_IS_CONTAINER(parent)) {
      gtk_container_add(GTK_CONTAINER(parent), w);
      parent_ = parent;
      gtk_widget_show(w);
      applyContainerLabel();
    }

    return static_cast<Derived &>(*this);
  }

  // Detaches the widget from its current parent, if any.
  Derived &detachFromParent() {
    GtkWidget *w = widget_;
    if (parent_ && w) {
      if (!gtk_widget_in_destruction(w) && GTK_IS_CONTAINER(parent_)) {
        gtk_container_remove(GTK_CONTAINER(parent_), w);
      }
      parent_ = nullptr;
    }
    return static_cast<Derived &>(*this);
  }

  Derived &selectAsCurrent() {
    GtkWidget *w = widget_;
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
      gtk_container_child_get(GTK_CONTAINER(parent_), w, "name", &name, nullptr);
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

  Derived &setLabel(std::string_view label) {
    label_.assign(label);
    applyContainerLabel();
    return static_cast<Derived &>(*this);
  }

 protected:
  GtkAttachable() = default;

  GtkWidget *parent_ = nullptr;
  GtkWidget *widget_ = nullptr;
  std::string label_;

  void applyContainerLabel() {
    GtkWidget *w = widget_;
    if (label_.empty() || !parent_ || !w) {
      return;
    }

    if (GTK_IS_NOTEBOOK(parent_)) {
      gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(parent_), w, label_.c_str());
    } else if (GTK_IS_STACK(parent_)) {
      gtk_container_child_set(GTK_CONTAINER(parent_), w, "name", label_.c_str(), nullptr);
    } else if (GTK_IS_ASSISTANT(parent_)) {
      gtk_assistant_set_page_title(GTK_ASSISTANT(parent_), w, label_.c_str());
    } else if (GTK_IS_EXPANDER(parent_)) {
      gtk_expander_set_label(GTK_EXPANDER(parent_), label_.c_str());
    }
  }

  // Can be called multiple times.
  // Last call is kept in widget_
  void trackWidget(GtkWidget *w) {
    if (!w) {
      return;
    }
    widget_ = w;
  }
};
