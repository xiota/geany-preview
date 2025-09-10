// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cctype>
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

}  // namespace StringUtils
