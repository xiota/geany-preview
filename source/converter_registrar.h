// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "converter.h"
#include "document.h"

class ConverterRegistrar final {
 public:
  ConverterRegistrar();

  // Registration API
  void registerConverter(std::unique_ptr<Converter> converter);
  void registerConverter(std::string converter_key, std::unique_ptr<Converter> converter);
  void registerFiletype(std::string converter_key, std::string geany_filetype_name);
  void registerExtension(std::string converter_key, std::string extension);

  // Convenience: register filetype + extensions together
  void registerFormatAliases(
      const std::string &converter_key,
      const std::string &filetype_name,
      std::initializer_list<std::string> extensions
  );

  // Resolve converter key from a Document using filetype name or extension fallback
  std::string resolveConverterKey(const Document &document) const;

  Converter *getConverter(const std::string &key) const {
    auto it = converters_.find(key);
    return it != converters_.end() ? it->second.get() : nullptr;
  }

  Converter *getConverter(const Document &document) const {
    auto key = resolveConverterKey(document);
    return getConverter(key);
  }

 private:
  std::string keyForFiletype(const std::string &geany_filetype_name) const;
  void registerBuiltins();
  void registerDefaultMappings();

  std::unordered_map<std::string, std::unique_ptr<Converter>> converters_;

  // Map from Geany filetype display name -> converter key
  std::unordered_map<std::string, std::string> type_to_converter_;
  // Map from lowercase file extension (including '.') -> converter key
  std::unordered_map<std::string, std::string> ext_to_converter_;
};
