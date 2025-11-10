// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "document_geany.h"

#include <functional>  // std::hash
#include <string>

#include <Scintilla.h>
#include <geany/document.h>

DocumentGeany::DocumentGeany(GeanyDocument *geany_document) : geany_document_(geany_document) {
  updateFilePath().updateFiletypeName().updateEncodingName();
}

std::string_view DocumentGeany::textView() const {
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

  return std::string_view(buffer_ptr, length);
}

std::string DocumentGeany::text() const {
  auto view = textView();
  return std::string{ view };
}

DocumentGeany &DocumentGeany::updateFilePath() {
  if (geany_document_ && geany_document_->real_path) {
    file_name_ = geany_document_->real_path;
  } else {
    file_name_.clear();
  }
  return *this;
}

DocumentGeany &DocumentGeany::updateFiletypeName() {
  if (geany_document_ && geany_document_->file_type && geany_document_->file_type->name) {
    filetype_name_ = geany_document_->file_type->name;
  } else {
    filetype_name_.clear();
  }
  return *this;
}

DocumentGeany &DocumentGeany::updateEncodingName() {
  if (geany_document_ && geany_document_->encoding) {
    encoding_name_ = geany_document_->encoding;
  } else {
    encoding_name_.clear();
  }
  return *this;
}
