/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "auxiliary.hxx"

#include <algorithm>
#include <fstream>
#include <iostream>

std::string & ltrim_inplace(std::string & s, const char * t) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

std::string & rtrim_inplace(std::string & s, const char * t) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

std::string & trim_inplace(std::string & s, const char * t) {
  return ltrim_inplace(rtrim_inplace(s, t), t);
}

std::string & replace_all_inplace(
    std::string & subject, const std::string_view & search, const std::string_view & replace
) {
  std::size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
  return subject;
}

std::string ws_ltrim(std::string s) {
  return ltrim_inplace(s, FOUNTAIN_WHITESPACE);
}

std::string ws_rtrim(std::string s) {
  return rtrim_inplace(s, FOUNTAIN_WHITESPACE);
}

std::string ws_trim(std::string s) {
  return trim_inplace(s, FOUNTAIN_WHITESPACE);
}

std::string replace_all(
    std::string subject, const std::string_view & search, const std::string_view & replace
) {
  return replace_all_inplace(subject, search, replace);
}

bool begins_with(const std::string & input, const std::string & match) {
  return !strncmp(input.c_str(), match.c_str(), match.length());
}

std::vector<std::string> split_string(
    const std::string_view & str, const std::string_view & delimiter
) {
  std::vector<std::string> result;

  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    result.emplace_back(str.substr(prev, pos - prev));
    prev = pos + delimiter.length();
  }

  // to get the last substring
  // (or entire string if delimiter is not found)
  result.emplace_back(str.substr(prev));

  return result;
}

std::vector<std::string> split_lines(const std::string_view & s) {
  return split_string(s, "\n");
}

std::string & to_upper_inplace(std::string & s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return std::toupper(c);
  });
  return s;
}

std::string & to_lower_inplace(std::string & s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return s;
}

std::string to_upper(std::string s) {
  return to_upper_inplace(s);
}
std::string to_lower(std::string s) {
  return to_lower_inplace(s);
}

struct HtmlEntities {
  std::string entity;
  std::string value;
};

static HtmlEntities entities[] = {
    {"&#38;", "&" },
    {"&#42;", "*" },
    {"&#95;", "_" },
    {"&#58;", ":" },
    {"&#91;", "[" },
    {"&#93;", "]" },
    {"&#92;", "\\"},
    {"&#60;", "<" },
    {"&#62;", ">" },
    {"&#46;", "." }
};

bool is_upper(const std::string_view & s) {
  return std::all_of(s.begin(), s.end(), [](unsigned char c) { return !std::islower(c); });
}

std::string & encode_entities_inplace(std::string & input, const bool bProcessAllEntities) {
  for (std::size_t pos = 0; pos < input.length();) {
    switch (input[pos]) {
      case '&':
        input.replace(pos, 1, "&#38;");
        pos += 5;
        break;
      case '<':
        input.replace(pos, 1, "&#60;");
        pos += 5;
        break;
      default:
        if (bProcessAllEntities) {
          for (auto e : entities) {
            if (e.value == input.substr(pos, 1)) {
              input.replace(pos, e.value.length(), e.entity);
              pos += e.entity.length() - 1;
              break;
            }
          }
        }
        ++pos;
        break;
    }
  }
  return input;
}

std::string encode_entities(std::string input, const bool bProcessAllEntities) {
  return encode_entities_inplace(input, bProcessAllEntities);
}

std::string & decode_entities_inplace(std::string & input) {
  for (std::size_t pos = 0; pos < input.length();) {
    switch (input[pos]) {
      case '&': {
        std::size_t end;
        if ((end = input.find(';', pos)) != std::string::npos) {
          std::string substr = input.substr(pos, end - pos + 1);
          for (auto e : entities) {
            if (substr == e.entity) {
              input.replace(pos, substr.length(), e.value);
              pos += e.value.length() - 1;
              break;
            }
          }
        }
        ++pos;
      } break;
      default:
        ++pos;
        break;
    }
  }

  return input;
}

std::string decode_entities(std::string input) {
  return decode_entities_inplace(input);
}

std::string cstr_assign(char * input) {
  if (input) {
    std::string output{input};
    free(input);
    return output;
  }
  return {};
}

std::vector<std::string> cstrv_assign(char ** input) {
  std::vector<std::string> output = cstrv_copy(input);

  if (input) {
    for (std::size_t i = 0; input[i] != nullptr; ++i) {
      free(input[i]);
    }
    free(input);
  }
  return output;
}

std::vector<std::string> cstrv_copy(const char * const * input) {
  std::vector<std::string> output;

  if (input) {
    for (std::size_t i = 0; input[i] != nullptr; ++i) {
      output.push_back(std::string{input[i]});
    }
  }
  return output;
}

// usage: (char **)cstrv_get().data()
std::vector<char *> cstrv_get(const std::vector<std::string> input) {
  std::vector<char *> output;
  for (std::size_t i = 0; i < input.size(); ++i) {
    output.push_back(const_cast<char *>(input[i].c_str()));
  }
  output.push_back(nullptr);
  return output;
}

std::string file_get_contents(const std::string & filename) {
  try {
    std::ifstream instream(filename, std::ios::in);
    std::string contents(
        (std::istreambuf_iterator<char>(instream)), (std::istreambuf_iterator<char>())
    );
    instream.close();
    return contents;
  } catch (...) {
    return {};
  }
}

bool file_set_contents(const std::string & filename, const std::string & contents) {
  try {
    std::ofstream outstream(filename, std::ios::out);
    copy(contents.begin(), contents.end(), std::ostream_iterator<char>(outstream));
    outstream.close();
    return true;
  } catch (...) {
    return false;
  }
}

std::vector<std::uint8_t> file_get_data(const std::string & filename) {
  try {
    std::ifstream instream(filename, std::ios::in | std::ios::binary);
    std::vector<std::uint8_t> contents(
        (std::istreambuf_iterator<char>(instream)), (std::istreambuf_iterator<char>())
    );
    instream.close();
    return contents;
  } catch (...) {
    return {};
  }
}

bool file_set_data(const std::string & filename, const std::vector<std::uint8_t> & contents) {
  try {
    std::ofstream outstream(filename, std::ios::out | std::ios::binary);
    copy(contents.begin(), contents.end(), std::ostream_iterator<char>(outstream));
    outstream.close();
    return true;
  } catch (...) {
    return false;
  }
}

void print_regex_error(std::regex_error & e, const char * file, const int line) {
  switch (e.code()) {
    case std::regex_constants::error_collate:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained an invalid collating "
          "element name.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_ctype:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained an invalid character"
          " class name.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_escape:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained an invalid escaped "
          "character, or a trailing escape.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_backref:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained an invalid back "
          "reference.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_brack:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained mismatched brackets "
          "([ and ]).\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_paren:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained mismatched parentheses "
          "(( and )).\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_brace:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained mismatched braces "
          "({ and }).\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_badbrace:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained an invalid range "
          "between braces ({ and }).\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_range:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained an invalid character "
          "range.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_space:
      fprintf(
          stderr,
          "%s:%d / %d: There was insufficient memory to convert the "
          "expression into a finite state machine.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_badrepeat:
      fprintf(
          stderr,
          "%s:%d / %d: The expression contained a repeat specifier "
          "(one of *?+{) that was not preceded by a valid regular "
          "expression.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_complexity:
      fprintf(
          stderr,
          "%s:%d / %d: The complexity of an attempted match against "
          "a regular expression exceeded a pre-set level.\n",
          file, line, e.code()
      );
      break;
    case std::regex_constants::error_stack:
      fprintf(
          stderr,
          "%s:%d / %d: There was insufficient memory to determine "
          "whether the regular expression could match the specified "
          "character sequence.\n",
          file, line, e.code()
      );
      break;
    default:
      fprintf(stderr, "%s:%d / %d: unknown regex error\n", file, line, e.code());
  }
}
