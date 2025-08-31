// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>
#include <gtk/gtk.h>

#include "converter_registrar.h"
#include "webview.h"

class PreviewPane final {
 public:
  PreviewPane(GtkWidget *notebook) : sidebar_notebook_(notebook) {
    attach();
  }

  ~PreviewPane() noexcept {
    detach();
  }

  PreviewPane &attach() {
    auto *webview_widget = webview_.widget();
    if (webview_widget && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      sidebar_page_number_ = gtk_notebook_append_page(
          GTK_NOTEBOOK(sidebar_notebook_), webview_widget, gtk_label_new("Preview")
      );
      gtk_widget_show_all(webview_widget);
      gtk_notebook_set_current_page(GTK_NOTEBOOK(sidebar_notebook_), sidebar_page_number_);
    }
    return *this;
  }

  PreviewPane &detach() {
    auto *webview_widget = webview_.widget();
    if (webview_widget && GTK_IS_NOTEBOOK(sidebar_notebook_)) {
      int idx = gtk_notebook_page_num(GTK_NOTEBOOK(sidebar_notebook_), webview_widget);
      if (idx >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(sidebar_notebook_), idx);
      }
    }
    return *this;
  }

  PreviewPane &update(GeanyDocument* geany_document = nullptr) {
    if (!geany_document) {
      geany_document = document_get_current();
    }

    Document document(geany_document);
    auto *converter = registrar_.getConverter(document);

    if (converter) {
      auto html = converter->toHtml(document.textView());
      webview_.loadHtml(html);
    } else {
      auto text = document.filetypeName() + ", " + document.encodingName();
      webview_.loadPlainText(text);
    }
    return *this;
  }

 private:
  GtkWidget *sidebar_notebook_;
  int sidebar_page_number_ = 0;

  WebView webview_;
  ConverterRegistrar registrar_;
};
