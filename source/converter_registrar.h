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
#include "converter_cmark_gfm.h"
#include "converter_pandoc.h"
#include "converter_passthrough.h"
#include "document.h"

class ConverterRegistrar final {
 public:
  ConverterRegistrar();

  Converter *getConverter(const std::string &key) const;
  Converter *getConverter(const Document &document) const;

 private:
  struct ConverterDef {
    std::string key;
    std::function<std::unique_ptr<Converter>()> factory;
    std::string filetype_name;
    std::vector<std::string> extensions;
  };

  // clang-format off
  inline static const ConverterDef converter_defs_[] = {
    // native
    { "html", [] { return std::make_unique<ConverterPassthrough>() ;}, "HTML", {".htm", ".html", ".shtml", ".xhtml"}},
    { "markdown", [] { return std::make_unique<ConverterCmarkGfm>() ;}, "Markdown", {".md", ".markdown", ".txt"}},

    // subprocess
    { "asciidoc", [] { return std::make_unique<ConverterAsciidoctor>() ;}, "Asciidoc", {".asciidoc", ".adoc", ".asc"}},

    // pandoc
    { "creole", [] { return std::make_unique<ConverterPandoc>("creole") ;}, "Creole Wiki", {".creole"}},
    { "docbook", [] { return std::make_unique<ConverterPandoc>("docbook") ;}, "DocBook", {".dbk"}},
    { "dokuwiki", [] { return std::make_unique<ConverterPandoc>("dokuwiki") ;}, "DokuWiki", {".dokuwiki", ".wiki"}},
    { "latex", [] { return std::make_unique<ConverterPandoc>("latex") ;}, "LaTeX", {".tex", ".latex"}},
    { "man", [] { return std::make_unique<ConverterPandoc>("man") ;}, "Unix Manpage", {".man"}},
    { "mediawiki", [] { return std::make_unique<ConverterPandoc>("mediawiki") ;}, "MediaWiki", {".mediawiki"}},
    { "org", [] { return std::make_unique<ConverterPandoc>("org") ;}, "Org mode", {".org"}},
    { "rst", [] { return std::make_unique<ConverterPandoc>("rst") ;}, "reStructuredText", {".rst"}},
    { "rtf", [] { return std::make_unique<ConverterPandoc>("rtf") ;}, "Rich Text Format", {".rtf"}},
    { "t2t", [] { return std::make_unique<ConverterPandoc>("t2t") ;}, "Txt2tags", {".t2t"}},
    { "textile", [] { return std::make_unique<ConverterPandoc>("textile") ;}, "Textile", {".textile"}},
    { "tikiwiki", [] { return std::make_unique<ConverterPandoc>("tikiwiki") ;}, "TikiWiki", {".tiki", ".tikiwiki"}},
    { "twiki", [] { return std::make_unique<ConverterPandoc>("twiki") ;}, "TWiki", {".twiki"}},
    { "vimwiki", [] { return std::make_unique<ConverterPandoc>("vimwiki") ;}, "Vimwiki", {".vw", ".vimwiki"}}
  };
  // clang-format on

  std::string resolveConverterKey(const Document &document) const;
  std::string keyForFiletype(const std::string &geany_filetype_name) const;
  static std::string normalizeExtension(const std::string &ext);

  std::unordered_map<std::string, std::unique_ptr<Converter>> converters_;
  std::unordered_map<std::string, std::string> type_to_converter_;
  std::unordered_map<std::string, std::string> ext_to_converter_;
};
