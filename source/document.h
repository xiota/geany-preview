// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>     // size_t
#include <functional>  // std::hash
#include <string>
#include <string_view>

#include <document.h>

class Document final {
 public:
  explicit Document(GeanyDocument *geany_document) : geany_document_(geany_document) {
    updateFileName().updateFiletypeName().updateEncodingName();
  }

  // Borrowed view into the live Scintilla buffer
  std::string_view textView() const;

  // Owning copy of the current text
  std::string text() const {
    auto view = textView();
    return std::string{view};
  }

  const std::string &fileName() const {
    return file_name_;
  }
  Document &updateFileName() {
    if (geany_document_ && geany_document_->real_path) {
      file_name_ = geany_document_->real_path;
    } else {
      file_name_.clear();
    }
    return *this;
  }

  const std::string &filetypeName() const {
    return filetype_name_;
  }
  Document &updateFiletypeName() {
    if (geany_document_ && geany_document_->file_type && geany_document_->file_type->name) {
      filetype_name_ = geany_document_->file_type->name;
    } else {
      filetype_name_.clear();
    }
    return *this;
  }

  const std::string &encodingName() const {
    return encoding_name_;
  }
  Document &updateEncodingName() {
    if (geany_document_ && geany_document_->encoding) {
      encoding_name_ = geany_document_->encoding;
    } else {
      encoding_name_.clear();
    }

    return *this;
  }

  size_t lastRenderHash() const {
    return last_render_hash_;
  }
  Document &setLastRenderHash(size_t hash) {
    last_render_hash_ = hash;
    return *this;
  }

  size_t computeHash() const {
    return std::hash<std::string_view>{}(textView());
  }

 private:
  GeanyDocument *geany_document_;
  std::string file_name_;
  std::string filetype_name_;
  std::string encoding_name_;

  size_t last_render_hash_ = 0;
};
