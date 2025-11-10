// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "document.h"

struct GeanyDocument;

class DocumentGeany final : public Document {
 public:
  explicit DocumentGeany(GeanyDocument *geany_document);

  std::string_view textView() const override;
  std::string text() const override;

  const std::string &filePath() const override {
    return file_name_;
  }
  const std::string &filetypeName() const override {
    return filetype_name_;
  }
  const std::string &encodingName() const override {
    return encoding_name_;
  }

  DocumentGeany &updateFilePath();
  DocumentGeany &updateFiletypeName();
  DocumentGeany &updateEncodingName();

 private:
  GeanyDocument *geany_document_;
  std::string file_name_;
  std::string filetype_name_;
  std::string encoding_name_;
};
