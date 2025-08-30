// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"

#include "converter_registrar.h"
#include "document.h"

void PreviewPane::update(GeanyDocument *geany_document) {
  if (!geany_document) {
    return;
  }

  Document document(geany_document);
  auto *conv = registrar_.getConverter(document);

  if (conv) {
    auto html = conv->toHtml(document.textView());
    webview_.loadHtml(html);
  } else {
    webview_.loadHtml(
        "<!doctype html><html><body><p>No converter registered.</p></body></html>"
    );
  }
}
