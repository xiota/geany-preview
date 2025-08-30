// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>     // size_t
#include <functional>  // std::hash
#include <string>
#include <string_view>

struct GeanyDocument;

class Document final {
 public:
  explicit Document(GeanyDocument *geany_document) : geany_document_(geany_document) {
    updateFiletypeName();
    updateFileName();
  }

  // Borrowed view into the live Scintilla buffer
  std::string_view textView() const;

  // Owning copy of the current text
  std::string text() const {
    auto view = textView();
    return std::string{view};
  }

  const std::string &filetypeName() const {
    return filetype_name_;
  }
  Document &updateFiletypeName();

  const std::string &fileName() const {
    return file_name_;
  }
  Document &updateFileName();

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
  std::string filetype_name_;
  std::string file_name_;
  size_t last_render_hash_ = 0;
};
