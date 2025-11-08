// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "converter.h"
#include "converter_asciidoctor.h"
#include "converter_cmark.h"
#include "converter_ftn2xml.h"
#include "converter_pandoc.h"
#include "converter_passthrough.h"
#include "document.h"
#include "util/string_utils.h"

using ConverterFactory = std::function<std::unique_ptr<Converter>()>;

class ConverterRegistrar final {
 public:
  ConverterRegistrar();

  Converter *getConverter(const std::string &key) const;
  Converter *getConverter(const Document &document) const;

  std::string getConverterKey(const std::string &alias) const;
  std::string getConverterKey(const Document &document) const;

 private:
  struct ConverterDef {
    std::string key;
    std::string filetype_name;
    std::function<std::unique_ptr<Converter>()> factory;
    std::vector<std::string> extensions;
    std::vector<std::string> mime_types;
  };

  // clang-format off
  inline static const ConverterDef converter_defs_[] = {
    // native
    { "fountain", "Fountain",
      [] { return std::make_unique<ConverterFtn2xml>(); },
      { ".ftn", ".fountain" },
      { "text/fountain", "text/x-fountain" } },

    { "html", "HTML",
      [] { return std::make_unique<ConverterPassthrough>(); },
      { ".htm", ".html", ".shtml", ".xhtml" },
      { "text/html", "application/xhtml+xml" } },

    { "markdown", "Markdown",
      [] { return std::make_unique<ConverterCmark>(); },
      { ".md", ".markdown", ".txt" },
      { "text/markdown", "text/x-markdown" } },

    // subprocess
    { "asciidoc", "Asciidoc",
      [] { return std::make_unique<ConverterAsciidoctor>(); },
      { ".asciidoc", ".adoc", ".asc" },
      { "text/asciidoc", "application/x-asciidoc" } },

    // pandoc
    { "creole", "Creole Wiki",
      [] { return std::make_unique<ConverterPandoc>("creole"); },
      { ".creole" },
      { "text/x-creole" } },

    { "docbook", "DocBook",
      [] { return std::make_unique<ConverterPandoc>("docbook"); },
      { ".dbk" },
      { "application/docbook+xml" } },

    { "dokuwiki", "DokuWiki",
      [] { return std::make_unique<ConverterPandoc>("dokuwiki"); },
      { ".dokuwiki", ".wiki" },
      { "text/x-dokuwiki" } },

    { "latex", "LaTeX",
      [] { return std::make_unique<ConverterPandoc>("latex"); },
      { ".tex", ".latex" },
      { "application/x-latex" } },

    { "man", "Unix Manpage",
      [] { return std::make_unique<ConverterPandoc>("man"); },
      { ".man" },
      { "text/troff", "application/x-troff-man" } },

    { "mediawiki", "MediaWiki",
      [] { return std::make_unique<ConverterPandoc>("mediawiki"); },
      { ".mediawiki" },
      { "text/mediawiki" } },

    { "org", "Org mode",
      [] { return std::make_unique<ConverterPandoc>("org"); },
      { ".org" },
      { "text/x-org" } },

    { "rst", "reStructuredText",
      [] { return std::make_unique<ConverterPandoc>("rst"); },
      { ".rst" },
      { "text/x-rst" } },

    { "rtf", "Rich Text Format",
      [] { return std::make_unique<ConverterPandoc>("rtf"); },
      { ".rtf" },
      { "application/rtf", "text/rtf" } },

    { "t2t", "Txt2tags",
      [] { return std::make_unique<ConverterPandoc>("t2t"); },
      { ".t2t" },
      { "text/x-txt2tags" } },

    { "textile", "Textile",
      [] { return std::make_unique<ConverterPandoc>("textile"); },
      { ".textile" },
      { "text/textile" } },

    { "tikiwiki", "TikiWiki",
      [] { return std::make_unique<ConverterPandoc>("tikiwiki"); },
      { ".tiki", ".tikiwiki" },
      { "text/x-tikiwiki" } },

    { "twiki", "TWiki",
      [] { return std::make_unique<ConverterPandoc>("twiki"); },
      { ".twiki" },
      { "text/x-twiki" } },

    { "vimwiki", "Vimwiki",
      [] { return std::make_unique<ConverterPandoc>("vimwiki"); },
      { ".vw", ".vimwiki" },
      { "text/x-vimwiki" } }
  };
  // clang-format on

 private:
  std::unordered_map<std::string, std::string> alias_to_key_;
  std::unordered_map<std::string, ConverterFactory> factories_;
  mutable std::unordered_map<std::string, std::unique_ptr<Converter>> instances_;

  static std::string normalizeAlias(std::string_view s) {
    return StringUtils::toLower(s);
  }

  static std::string normalizeExtension(std::string_view ext) {
    if (!ext.empty() && ext.front() == '.') {
      ext.remove_prefix(1);
    }
    return StringUtils::toLower(ext);
  }
};
