// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xdg_utils.h"

#include <cstdlib>
#include <string>
#include <unordered_set>

namespace XdgUtils {

std::string
expandEnvVars(const std::string &input, const std::unordered_set<std::string> &allowed_vars) {
  std::string out;
  out.reserve(input.size());

  for (size_t i = 0; i < input.size();) {
    if (input[i] == '$') {
      size_t start = i;
      std::string var;

      if (i + 1 < input.size() && input[i + 1] == '{') {
        size_t end = input.find('}', i + 2);
        if (end != std::string::npos) {
          var = input.substr(i + 2, end - (i + 2));
          i = end + 1;
        } else {
          out.push_back(input[i++]);
          continue;
        }
      } else {
        size_t j = i + 1;
        while (j < input.size() && (isalnum(input[j]) || input[j] == '_')) {
          j++;
        }
        var = input.substr(i + 1, j - (i + 1));
        i = j;
      }

      bool expand = allowed_vars.empty() || allowed_vars.count(var);
      if (expand) {
        const char *val = getenv(var.c_str());
        if (val) {
          out += val;
        }
      } else {
        out += input.substr(start, i - start);
      }
    } else {
      out.push_back(input[i++]);
    }
  }

  return out;
}

}  // namespace XdgUtils
