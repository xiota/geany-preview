// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_registrar.h"

#include <algorithm>
#include <filesystem>

ConverterRegistrar::ConverterRegistrar() {
  // prevent rehashing
  alias_to_key_.reserve(std::size(converter_defs_) * 5);

  for (const auto &def : converter_defs_) {
    // Store factory for later use
    factories_[def.key] = def.factory;

    auto addAlias = [&](std::string_view raw, auto normalizer) {
      auto alias = normalizer(raw);
      if (!alias_to_key_.contains(alias)) {
        alias_to_key_[alias] = def.key;
      }
    };

    // Canonical key
    addAlias(def.key, normalizeAlias);

    // Geany filetype name
    addAlias(def.filetype_name, normalizeAlias);

    // Extensions
    for (const auto &ext : def.extensions) {
      addAlias(ext, normalizeExtension);
    }

    // MIME types
    for (const auto &type : def.mime_types) {
      addAlias(type, normalizeAlias);
    }
  }
}

Converter *ConverterRegistrar::getConverter(const std::string &alias) const {
  std::string key = getConverterKey(alias);

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

std::string ConverterRegistrar::getConverterKey(const std::string &alias) const {
  auto it = alias_to_key_.find(normalizeAlias(alias));
  if (it != alias_to_key_.end()) {
    return it->second;
  }

  return {};
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
