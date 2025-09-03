// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_registrar.h"

#include <algorithm>
#include <filesystem>
#include <utility>

#include "converter_asciidoctor.h"
#include "converter_cmark_gfm.h"
#include "converter_pandoc.h"
#include "converter_passthrough.h"

ConverterRegistrar::ConverterRegistrar() {
  registerBuiltins();
  registerDefaultMappings();
}

void ConverterRegistrar::registerBuiltins() {
  // Native
  registerConverter("html", std::make_unique<ConverterPassthrough>());
  registerConverter("markdown", std::make_unique<ConverterCmarkGfm>());

  // Subprocess
  registerConverter("asciidoc", std::make_unique<ConverterAsciidoctor>());

  // Pandoc
  registerConverter(std::make_unique<ConverterPandoc>("creole"));
  registerConverter(std::make_unique<ConverterPandoc>("docbook"));
  registerConverter(std::make_unique<ConverterPandoc>("dokuwiki"));
  registerConverter(std::make_unique<ConverterPandoc>("latex"));
  registerConverter(std::make_unique<ConverterPandoc>("man"));
  registerConverter(std::make_unique<ConverterPandoc>("mediawiki"));
  registerConverter(std::make_unique<ConverterPandoc>("org"));
  registerConverter(std::make_unique<ConverterPandoc>("rst"));
  registerConverter(std::make_unique<ConverterPandoc>("rtf"));
  registerConverter(std::make_unique<ConverterPandoc>("t2t"));
  registerConverter(std::make_unique<ConverterPandoc>("textile"));
  registerConverter(std::make_unique<ConverterPandoc>("tikiwiki"));
  registerConverter(std::make_unique<ConverterPandoc>("twiki"));
  registerConverter(std::make_unique<ConverterPandoc>("vimwiki"));
}

void ConverterRegistrar::registerDefaultMappings() {
  // Native
  registerFormatAliases("html", "HTML", { ".htm", ".html", ".shtml", ".xhtml" });
  registerFormatAliases("markdown", "Markdown", { ".md", ".markdown", ".txt" });

  // Subprocess
  registerFormatAliases("asciidoc", "Asciidoc", { ".asciidoc", ".adoc", ".asc" });

  // Pandoc
  registerFormatAliases("creole", "Creole Wiki", { ".creole" });            // extension based
  registerFormatAliases("docbook", "DocBook", { ".dbk" });                  // extension based
  registerFormatAliases("dokuwiki", "DokuWiki", { ".dokuwiki", ".wiki" });  // extension based
  registerFormatAliases("latex", "LaTeX", { ".tex", ".latex" });
  registerFormatAliases("man", "Unix Manpage", { ".man" });           // extension based
  registerFormatAliases("mediawiki", "MediaWiki", { ".mediawiki" });  // extension based
  registerFormatAliases("org", "Org mode", { ".org" });
  registerFormatAliases("rst", "reStructuredText", { ".rst" });
  registerFormatAliases("rtf", "Rich Text Format", { ".rtf" });  // extension based
  registerFormatAliases("t2t", "Txt2tags", { ".t2t" });
  registerFormatAliases("textile", "Textile", { ".textile" });              // extension based
  registerFormatAliases("tikiwiki", "TikiWiki", { ".tiki", ".tikiwiki" });  // extension based
  registerFormatAliases("twiki", "TWiki", { ".twiki" });                    // extension based
  registerFormatAliases("vimwiki", "Vimwiki", { ".vw", ".vimwiki" });       // extension based
}

void ConverterRegistrar::registerConverter(std::unique_ptr<Converter> c) {
  auto key = std::string{ c->id() };
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

std::string ConverterRegistrar::resolveConverterKey(const Document &document) const {
  // 1. Try Geany filetype mapping
  std::string name = document.filetypeName();
  auto it = type_to_converter_.find(name);
  if (it != type_to_converter_.end()) {
    return it->second;
  }

  // 2. Try extension mapping
  namespace fs = std::filesystem;
  std::string ext = fs::path(document.fileName()).extension().string();
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
