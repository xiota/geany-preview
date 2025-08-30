// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_pane.h"
#include "converter_registrar.h"
#include "document.h"

void PreviewPane::update(GeanyDocument *gdoc) {
  if (!gdoc) {
    return;  // no doc, nothing to preview
  }

  Document doc = makeDocumentFromGeany(gdoc);
  auto *conv = registrar_.getConverter(doc);

  if (conv) {
    std::string html = conv->toHtml(doc.text());
    webview_.loadHtml(html);
  } else {
    webview_.loadHtml(
        "<!doctype html><html><body><p>No converter registered.</p></body></html>"
    );
  }
}
