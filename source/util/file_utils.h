// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace FileUtils {

inline std::string readFileToString(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

}  // namespace FileUtils
