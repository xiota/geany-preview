// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "document_geany.h"
#include "util/string_utils.h"

class ConverterPreprocessor {
 public:
  ConverterPreprocessor() = default;
  explicit ConverterPreprocessor(const Document &doc, std::size_t max_incomplete)
      : max_incomplete_(max_incomplete) {
    preprocess(doc);
  }

  void preprocess(const Document &doc) {
    headers_.clear();
    type_.clear();
    body_ = {};
    splitDocument(doc);
  }

  const std::vector<std::pair<std::string_view, std::string_view>> &headers() const noexcept {
    return headers_;
  }
  std::string_view body() const noexcept {
    return body_;
  }
  const std::string &type() const noexcept {
    return type_;
  }

  std::string headersToHtml() const {
    if (headers_.empty()) {
      return {};
    }

    std::string html;
    html += "<div class=\"headers\">\n";
    for (auto &[key, value] : headers_) {
      bool incomplete = key.empty() || value.empty();
      html += "  <div class=\"header-line";
      if (incomplete) {
        html += " incomplete";
      }
      html += "\">";

      // Key
      html += "<span class=\"header-key\">";
      html += key.empty() ? "(missing key)" : StringUtils::escapeHtml(key);
      html += "</span>: ";

      // Value
      html += "<span class=\"header-value\">";
      html += value.empty() ? "&mdash;" : StringUtils::escapeHtml(value);
      html += "</span>";

      html += "</div>\n";
    }
    html += "</div>\n";
    return html;
  }

 private:
  static bool isValidHeaderKeyLine(std::string_view line, std::size_t colon_pos) {
    // Reject if starts with space/tab
    if (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
      return false;
    }

    // Reject if key portion contains space/tab
    std::string_view key_part = line.substr(0, colon_pos);
    if (key_part.find(' ') != std::string_view::npos ||
        key_part.find('\t') != std::string_view::npos) {
      return false;
    }

    // Lowercase for comparison
    auto lower_key = StringUtils::toLower(key_part);

    // Known protocols
    static const std::vector<std::string> protocols = { "http", "https",  "file",
                                                        "ftp",  "mailto", "data" };

    // Check char after colon
    char after_colon = (colon_pos + 1 < line.size()) ? line[colon_pos + 1] : '\0';
    char after_colon2 = (colon_pos + 2 < line.size()) ? line[colon_pos + 2] : '\0';

    // Reject known protocol + no space after colon
    for (const auto &proto : protocols) {
      if (lower_key == proto && after_colon != ' ' && after_colon != '\t') {
        return false;
      }
    }

    // Reject colon followed immediately by "//"
    if (after_colon == '/' && after_colon2 == '/') {
      return false;
    }

    return true;
  }

  void splitDocument(const Document &doc) {
    std::string_view view = doc.textView();
    std::size_t pos = 0;
    std::size_t incompleteCount = 0;

    headers_.clear();
    type_.clear();
    body_ = {};
    headers_.reserve(8);

    while (pos < view.size()) {
      // Find end of current line
      std::size_t line_end = view.find_first_of("\r\n", pos);
      std::string_view line = (line_end == std::string_view::npos)
                                  ? view.substr(pos)
                                  : view.substr(pos, line_end - pos);
      line = StringUtils::trimWhitespaceView(line);

      // Blank line → end of header block
      if (line.empty()) {
        if (line_end != std::string_view::npos) {
          if (view[line_end] == '\r') {
            ++line_end;
          }
          if (line_end < view.size() && view[line_end] == '\n') {
            ++line_end;
          }
        }
        pos = line_end;
        break;
      }

      // Parse header line
      std::size_t colon_pos = line.find(':');
      if (colon_pos == std::string_view::npos || !isValidHeaderKeyLine(line, colon_pos)) {
        if (headers_.empty()) {
          // First line is not a valid header → bail early
          headers_.clear();
          type_.clear();
          body_ = view;
          return;
        }
        // Treat as incomplete: whole line is key, empty value
        headers_.emplace_back(StringUtils::trimWhitespaceView(line), std::string_view{});
        ++incompleteCount;
      } else {
        std::string_view k = StringUtils::trimWhitespaceView(line.substr(0, colon_pos));
        std::string_view v = StringUtils::trimWhitespaceView(line.substr(colon_pos + 1));

        if (k.empty() || v.empty()) {
          ++incompleteCount;
        } else if (StringUtils::toLower(k) == "format" ||
                   StringUtils::toLower(k) == "content-type") {
          // Take only MIME type before ';'
          std::string_view mime = v.substr(0, v.find(';'));
          mime = StringUtils::trimWhitespaceView(mime);
          type_ = StringUtils::toLower(mime);
        }
        headers_.emplace_back(k, v);
      }

      if (incompleteCount > max_incomplete_) {
        headers_.clear();
        type_.clear();
        body_ = view;
        return;
      }

      // Advance to next line
      if (line_end == std::string_view::npos) {
        pos = view.size();
        break;
      }
      if (view[line_end] == '\r') {
        ++line_end;
      }
      if (line_end < view.size() && view[line_end] == '\n') {
        ++line_end;
      }
      pos = line_end;
    }

    // Body is whatever remains
    body_ = (pos < view.size()) ? view.substr(pos) : std::string_view{};
  }

 private:
  std::vector<std::pair<std::string_view, std::string_view>> headers_;
  std::string type_;
  std::string_view body_;

  std::size_t max_incomplete_ = 3;
};
