// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "document.h"

#include <geanyplugin.h>

#include <Scintilla.h>

Document &Document::updateFiletypeName() {
  if (geany_document_ && geany_document_->file_type && geany_document_->file_type->name) {
    filetype_name_ = geany_document_->file_type->name;
  } else {
    filetype_name_.clear();
  }
  return *this;
}

Document &Document::updateFileName() {
  if (geany_document_ && geany_document_->real_path) {
    file_name_ = geany_document_->real_path;
  } else {
    file_name_.clear();
  }
  return *this;
}

std::string_view Document::textView() const {
  if (!geany_document_ || !geany_document_->editor) {
    return {};
  }

  ScintillaObject *sci = geany_document_->editor->sci;
  if (!sci) {
    return {};
  }

  sptr_t length = sci_get_length(sci);
  if (length == 0) {
    return {};
  }

  const char *buffer_ptr = reinterpret_cast<const char *>(
      scintilla_send_message(sci, SCI_GETCHARACTERPOINTER, 0, 0)
  );

  return std::string_view(buffer_ptr, static_cast<size_t>(length));
}
