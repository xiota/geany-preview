// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "document.h"

#include <Scintilla.h>

std::string_view Document::textView() const {
  if (!geany_document_ || !geany_document_->editor) {
    return {};
  }

  ScintillaObject *sci = geany_document_->editor->sci;
  if (!sci) {
    return {};
  }

  auto length = static_cast<size_t>(scintilla_send_message(sci, SCI_GETLENGTH, 0, 0));
  if (length == 0) {
    return {};
  }

  auto *buffer_ptr = reinterpret_cast<const char *>(
      scintilla_send_message(sci, SCI_GETCHARACTERPOINTER, 0, 0)
  );

  return std::string_view(buffer_ptr, static_cast<size_t>(length));
}
