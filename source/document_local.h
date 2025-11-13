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
      : filetype_name_(""), encoding_name_("UTF-8") {
    std::error_code ec;
    std::filesystem::path p = path;
    p = p.lexically_normal();
    if (!p.is_absolute()) {
      p = std::filesystem::absolute(p, ec);
      ec.clear();
    }
    if (std::filesystem::exists(p)) {
      file_path_ = p.string();
    } else {
      file_path_.clear();  // invalid / non-existent
    }
  }

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
    if (loaded_) {
      return;
    }
    loaded_ = true;

    if (file_path_.empty()) {
      text_cache_.clear();
      return;
    }

    text_cache_ = FileUtils::readFileToString(file_path_);
    if (text_cache_.empty()) {
    }
  }

  std::string file_path_;
  std::string filetype_name_;
  std::string encoding_name_;
  mutable std::string text_cache_;
  mutable bool loaded_ = false;
};
