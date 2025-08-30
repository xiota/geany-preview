// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <filesystem>
#include <utility>

#include "converter_cmark_gfm.h"
#include "converter_passthrough.h"
#include "converter_registrar.h"

ConverterRegistrar::ConverterRegistrar() {
  registerBuiltins();
  registerDefaultMappings();
}

void ConverterRegistrar::registerBuiltins() {
  registerConverter("html", std::make_unique<ConverterPassthrough>());
  registerConverter("markdown", std::make_unique<ConverterCmarkGfm>());
}

void ConverterRegistrar::registerDefaultMappings() {
  registerFormatAliases("html", "HTML", {".htm", ".html", ".shtml", ".xhtml"});
  registerFormatAliases("markdown", "Markdown", {".md", ".markdown"});
}

void ConverterRegistrar::registerConverter(std::unique_ptr<Converter> c) {
  auto key = std::string{c->id()};
  converters_[key] = std::move(c);
}

void ConverterRegistrar::registerConverter(
    std::string converter_key,
    std::unique_ptr<Converter> c
) {
  converters_[std::move(converter_key)] = std::move(c);
}

void ConverterRegistrar::registerFiletype(
    std::string converter_key,
    std::string geany_filetype_name
) {
  type_to_converter_[std::move(geany_filetype_name)] = std::move(converter_key);
}

void ConverterRegistrar::registerExtension(std::string converter_key, std::string extension) {
  // Store lowercase version of extension for matching
  std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  ext_to_converter_[std::move(extension)] = std::move(converter_key);
}

void ConverterRegistrar::registerFormatAliases(
    const std::string &converter_key,
    const std::string &filetype_name,
    std::initializer_list<std::string> extensions
) {
  registerFiletype(converter_key, filetype_name);
  for (const auto &ext : extensions) {
    registerExtension(converter_key, ext);
  }
}

std::string ConverterRegistrar::resolveConverterKey(const Document &doc) const {
  // 1. Try Geany filetype mapping
  std::string name = doc.filetypeName();
  auto it = type_to_converter_.find(name);
  if (it != type_to_converter_.end()) {
    return it->second;
  }

  // 2. Try extension mapping
  namespace fs = std::filesystem;
  std::string ext = fs::path(doc.fileName()).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  auto eit = ext_to_converter_.find(ext);
  if (eit != ext_to_converter_.end()) {
    return eit->second;
  }
  return {};
}

std::string ConverterRegistrar::keyForFiletype(const std::string &geany_filetype_name) const {
  auto it = type_to_converter_.find(geany_filetype_name);
  if (it != type_to_converter_.end()) {
    return it->second;
  }
  return {};  // Unknown
}
