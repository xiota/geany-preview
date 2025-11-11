// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "document.h"

#include <filesystem>
#include <string>

#include "util/file_utils.h"

class DocumentLocal final : public Document {
 public:
  explicit DocumentLocal(const std::filesystem::path &path)
      : file_path_(path.string()), filetype_name_(""), encoding_name_("UTF-8") {}

  std::string_view textView() const override {
    ensureLoaded();
    return text_cache_;
  }

  std::string text() const override {
    ensureLoaded();
    return text_cache_;
  }

  const std::string &filePath() const override {
    return file_path_;
  }

  const std::string &filetypeName() const override {
    return filetype_name_;
  }

  const std::string &encodingName() const override {
    return encoding_name_;
  }

 private:
  void ensureLoaded() const {
    if (!loaded_) {
      text_cache_ = FileUtils::readFileToString(file_path_);
      loaded_ = true;
    }
  }

  std::string file_path_;
  std::string filetype_name_;
  std::string encoding_name_;
  mutable std::string text_cache_;
  mutable bool loaded_ = false;
};
