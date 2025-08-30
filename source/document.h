// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>     // size_t
#include <functional>  // std::hash
#include <string>

class Document {
 public:
  Document() = default;
  explicit Document(
      std::string text,
      std::string filetype_name = "None",
      std::string file_name = ""
  )
      : text_(std::move(text)),
        filetype_name_(std::move(filetype_name)),
        file_name_(std::move(file_name)) {}

  // Content
  const std::string &text() const {
    return text_;
  }
  void setText(std::string text) {
    text_ = std::move(text);
  }

  // Geany filetype name (e.g., "Markdown", "HTML").
  const std::string &filetypeName() const {
    return filetype_name_;
  }
  void setFiletypeName(std::string name) {
    filetype_name_ = std::move(name);
  }

  // Optional file name for dispatch or display
  const std::string &fileName() const {
    return file_name_;
  }
  void setFileName(std::string name) {
    file_name_ = std::move(name);
  }

  // Detect changes
  size_t lastRenderHash() const {
    return last_render_hash_;
  }
  void setLastRenderHash(size_t hash) {
    last_render_hash_ = hash;
  }

  size_t computeHash() const {
    return std::hash<std::string>{}(text_);
  }

 private:
  std::string text_;
  std::string filetype_name_;
  std::string file_name_;

  size_t last_render_hash_ = 0;
};
