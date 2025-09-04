// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_registrar.h"

#include <algorithm>
#include <filesystem>

ConverterRegistrar::ConverterRegistrar() {
  for (const auto &def : converter_defs_) {
    converters_[def.key] = def.factory();
    type_to_converter_[def.filetype_name] = def.key;
    for (const auto &ext : def.extensions) {
      std::string normalized = normalizeExtension(ext);
      ext_to_converter_[normalized] = def.key;
    }
  }
}

Converter *ConverterRegistrar::getConverter(const std::string &key) const {
  auto it = converters_.find(key);
  return it != converters_.end() ? it->second.get() : nullptr;
}

Converter *ConverterRegistrar::getConverter(const Document &document) const {
  auto key = getConverterKey(document);
  return getConverter(key);
}

std::string ConverterRegistrar::getConverterKey(const Document &document) const {
  std::string name = document.filetypeName();
  auto it = type_to_converter_.find(name);
  if (it != type_to_converter_.end()) {
    return it->second;
  }

  namespace fs = std::filesystem;
  std::string ext = fs::path(document.fileName()).extension().string();
  std::string normalized = normalizeExtension(ext);
  auto eit = ext_to_converter_.find(normalized);
  if (eit != ext_to_converter_.end()) {
    return eit->second;
  }

  return {};
}

std::string ConverterRegistrar::getConverterKeyForFiletype(
    const std::string &geany_filetype_name
) const {
  auto it = type_to_converter_.find(geany_filetype_name);
  return it != type_to_converter_.end() ? it->second : "";
}

std::string ConverterRegistrar::normalizeExtension(const std::string &ext) {
  std::string result = ext;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return result;
}
