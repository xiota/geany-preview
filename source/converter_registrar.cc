// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_registrar.h"

#include <algorithm>
#include <filesystem>

ConverterRegistrar::ConverterRegistrar() {
  // prevent rehashing
  alias_to_key_.reserve(std::size(converter_defs_) * 4);

  for (const auto &def : converter_defs_) {
    // Store factory for later use
    factories_[def.key] = def.factory;

    // Canonical key
    auto alias = normalizeAlias(def.key);
    if (!alias_to_key_.contains(alias)) {
      alias_to_key_[alias] = def.key;
    }

    // Geany filetype name
    alias = normalizeAlias(def.filetype_name);
    if (!alias_to_key_.contains(alias)) {
      alias_to_key_[alias] = def.key;
    }

    // Extensions
    for (const auto &ext : def.extensions) {
      alias = normalizeExtension(ext);
      if (!alias_to_key_.contains(alias)) {
        alias_to_key_[alias] = def.key;
      }
    }
  }
}

Converter *ConverterRegistrar::getConverter(const std::string &key) const {
  // Check if instance already exists
  auto it = instances_.find(key);
  if (it != instances_.end()) {
    return it->second.get();
  }

  // Create new instance if factory exists
  auto fit = factories_.find(key);
  if (fit != factories_.end()) {
    auto instance = fit->second();
    auto *ptr = instance.get();
    instances_[key] = std::move(instance);
    return ptr;
  }

  return nullptr;
}

Converter *ConverterRegistrar::getConverter(const Document &document) const {
  auto key = getConverterKey(document);
  return getConverter(key);
}

std::string ConverterRegistrar::getConverterKey(const Document &document) const {
  // Try filetype name
  auto it = alias_to_key_.find(normalizeAlias(document.filetypeName()));
  if (it != alias_to_key_.end()) {
    return it->second;
  }

  // Try extension
  namespace fs = std::filesystem;
  std::string ext = fs::path(document.fileName()).extension().string();
  auto eit = alias_to_key_.find(normalizeExtension(ext));
  if (eit != alias_to_key_.end()) {
    return eit->second;
  }

  return {};
}

std::string ConverterRegistrar::getConverterKeyForFiletype(
    const std::string &geany_filetype_name
) const {
  auto it = alias_to_key_.find(normalizeAlias(geany_filetype_name));
  return it != alias_to_key_.end() ? it->second : "";
}
