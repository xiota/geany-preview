/*
 * C++ Fountain Parser - auxiliary functions
 * Copyright 2021 xiota
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#pragma once

#include <algorithm>
#include <regex>
#include <string>
#include <vector>

std::string &ltrim(std::string &s, char const *t = " \t\n\r\f\v") {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

std::string &rtrim(std::string &s, char const *t = " \t\n\r\f\v") {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

std::string &trim(std::string &s, char const *t = " \t\n\r\f\v") {
  return ltrim(rtrim(s, t), t);
}

std::string ws_ltrim(std::string s) { return ltrim(s, " \t\n\r\f\v"); }

std::string ws_rtrim(std::string s) { return rtrim(s, " \t\n\r\f\v"); }

std::string ws_trim(std::string s) { return trim(s, " \t\n\r\f\v"); }

bool begins_with(std::string const &input, char const *match) {
  return !strncmp(input.c_str(), match, strlen(match));
}

std::vector<std::string> split_string(std::string const &str,
                                      std::string const &delimiter) {
  std::vector<std::string> strings;

  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    strings.push_back(str.substr(prev, pos - prev));
    prev = pos + 1;
  }

  // To get the last substring (or only, if delimiter is not found)
  strings.push_back(str.substr(prev));

  return strings;
}

std::vector<std::string> split_lines(std::string const &s) {
  return split_string(s, "\n");
}

std::string to_upper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return s;
}

std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

bool is_upper(std::string const &s) {
  return std::all_of(s.begin(), s.end(),
                     [](unsigned char c) { return !std::islower(c); });
}

void print_regex_error(std::regex_error &e) {
  switch (e.code()) {
    case std::regex_constants::error_collate:
      printf(
          "%d: The expression contained an invalid collating element name.\n",
          e.code());
      break;
    case std::regex_constants::error_ctype:
      printf("%d: The expression contained an invalid character class name.\n",
             e.code());
      break;
    case std::regex_constants::error_escape:
      printf(
          "%d: The expression contained an invalid escaped character, "
          "or a trailing escape.\n",
          e.code());
      break;
    case std::regex_constants::error_backref:
      printf("%d: The expression contained an invalid back reference.\n",
             e.code());
      break;
    case std::regex_constants::error_brack:
      printf("%d: The expression contained mismatched brackets ([ and ]).\n",
             e.code());
      break;
    case std::regex_constants::error_paren:
      printf("%d: The expression contained mismatched parentheses (( and )).\n",
             e.code());
      break;
    case std::regex_constants::error_brace:
      printf("%d: The expression contained mismatched braces ({ and }).\n",
             e.code());
      break;
    case std::regex_constants::error_badbrace:
      printf(
          "%d: The expression contained an invalid range between braces "
          "({ and }).\n",
          e.code());
      break;
    case std::regex_constants::error_range:
      printf("%d: The expression contained an invalid character range.\n",
             e.code());
      break;
    case std::regex_constants::error_space:
      printf(
          "%d: There was insufficient memory to convert the expression "
          "into a finite state machine.\n",
          e.code());
      break;
    case std::regex_constants::error_badrepeat:
      printf(
          "%d: The expression contained a repeat specifier (one of *?+{) that "
          "was not preceded by a valid regular expression.\n",
          e.code());
      break;
    case std::regex_constants::error_complexity:
      printf(
          "%d: The complexity of an attempted match against a regular "
          "expression exceeded a pre-set level.\n",
          e.code());
      break;
    case std::regex_constants::error_stack:
      printf(
          "%d: There was insufficient memory to determine whether the regular "
          "expression could match the specified character sequence.\n",
          e.code());
      break;
    default:
      printf("%d: regex error in parseNodeText\n", e.code());
  }
}
