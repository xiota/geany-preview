// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

namespace StringUtils {

inline std::string toLower(std::string_view sv) {
  std::string s(sv);
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

inline std::string trimWhitespace(std::string_view sv) {
  const char *ws = " \t\r\n";
  auto b = sv.find_first_not_of(ws);
  if (b == std::string_view::npos) {
    return {};
  }
  auto e = sv.find_last_not_of(ws);
  return std::string(sv.substr(b, e - b + 1));
}

inline std::string_view trimWhitespaceView(std::string_view sv) {
  const char *ws = " \t\r\n";
  auto b = sv.find_first_not_of(ws);
  if (b == std::string_view::npos) {
    return {};
  }
  auto e = sv.find_last_not_of(ws);
  return sv.substr(b, e - b + 1);
}

inline std::string replaceAll(std::string s, const std::string &from, const std::string &to) {
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.length(), to);
    pos += to.length();
  }
  return s;
}

inline std::string escapeHtml(std::string_view sv) {
  std::string out;
  out.reserve(sv.size() * 2);  // reasonable headroom for most cases
  for (char c : sv) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

inline std::string randomHex(std::size_t length) {
  static thread_local std::mt19937 rng{ std::random_device{}() };
  static std::uniform_int_distribution<int> dist(0, 15);
  std::string out;
  out.reserve(length);
  for (std::size_t i = 0; i < length; ++i) {
    out.push_back("0123456789abcdef"[dist(rng)]);
  }
  return out;
}

}  // namespace StringUtils
