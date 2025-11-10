// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>  // size_t
#include <string>
#include <string_view>

class Document {
 public:
  virtual ~Document() = default;

  // Core interface
  virtual std::string_view textView() const = 0;
  virtual std::string text() const = 0;

  virtual const std::string &filePath() const = 0;
  virtual const std::string &filetypeName() const = 0;
  virtual const std::string &encodingName() const = 0;

  virtual size_t computeHash() const {
    return std::hash<std::string_view>{}(textView());
  }

  // Render tracking (shared across backends)
  size_t lastRenderHash() const {
    return last_render_hash_;
  }
  void setLastRenderHash(size_t hash) {
    last_render_hash_ = hash;
  }

 protected:
  size_t last_render_hash_ = 0;
};
