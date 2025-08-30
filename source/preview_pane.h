// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "converter_registrar.h"
#include "gtk_attachable.h"
#include "webview.h"

#include <Scintilla.h>
#include <ScintillaWidget.h>
#include <sciwrappers.h>

#include <geanyplugin.h>
#include <gtk/gtk.h>

inline std::string getDocumentText(GeanyDocument *gdoc) {
  if (!gdoc || !gdoc->editor || !gdoc->editor->sci) {
    return {};
  }

  ScintillaObject *sci = gdoc->editor->sci;
  sptr_t length = sci_get_length(sci);

  std::string text;
  text.resize(static_cast<size_t>(length) + 1, '\0');

  Sci_TextRange tr;
  tr.chrg.cpMin = 0;
  tr.chrg.cpMax = length;
  tr.lpstrText = text.data();
  scintilla_send_message(sci, SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&tr));

  return text;
}

class PreviewPane : public GtkAttachable<PreviewPane> {
 public:
  PreviewPane()
      : GtkAttachable("Preview"), container_(gtk_scrolled_window_new(nullptr, nullptr)) {
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(container_), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
    );

    webview_.loadHtml("<h1>Hello from GTK3!</h1>");
    webview_.attachToParent(container_);
  }

  GtkWidget *widget() const override {
    return container_;
  }

  void update(GeanyDocument *doc);

 private:
  GtkWidget *container_;
  WebView webview_;
  ConverterRegistrar registrar_;

  Document makeDocumentFromGeany(GeanyDocument *gdoc) {
    if (!gdoc || !gdoc->file_type) {
      return {};
    }
    return Document(
        getDocumentText(gdoc),
        (gdoc->file_type->name ? gdoc->file_type->name : ""),
        (gdoc->file_name ? gdoc->file_name : "")
    );
  }
};
