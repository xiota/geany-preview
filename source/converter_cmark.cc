// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_cmark.h"

#include <cstdlib>  // for free()
#include <cstring>  // for std::strlen
#include <memory>
#include <string>
#include <string_view>

#ifdef HAVE_CMARK_GFM
#  include <cmark-gfm-core-extensions.h>
#  include <cmark-gfm.h>
#else
#  include <cmark.h>
#endif

namespace {

#ifdef HAVE_CMARK_GFM
// Helper to attach a GFM extension by name
void add_extension(cmark_parser *parser, const char *name) {
  if (auto *ext = cmark_find_syntax_extension(name)) {
    cmark_parser_attach_syntax_extension(parser, ext);
  }
}
#endif

}  // namespace

std::string_view ConverterCmark::toHtml(std::string_view source) {
  int options = CMARK_OPT_SOURCEPOS | CMARK_OPT_SMART;

#ifdef HAVE_CMARK_GFM
  options |= CMARK_OPT_TABLE_PREFER_STYLE_ATTRIBUTES | CMARK_OPT_FOOTNOTES;
  cmark_gfm_core_extensions_ensure_registered();
#endif

  cmark_parser *parser = cmark_parser_new(options);

#ifdef HAVE_CMARK_GFM
  add_extension(parser, "table");
  add_extension(parser, "strikethrough");
  add_extension(parser, "autolink");
  add_extension(parser, "tagfilter");
  add_extension(parser, "tasklist");
#endif

  cmark_parser_feed(parser, source.data(), source.size());
  cmark_node *document = cmark_parser_finish(parser);

#ifdef HAVE_CMARK_GFM
  cmark_llist *exts = cmark_parser_get_syntax_extensions(parser);
  char *html_cstr = cmark_render_html(document, options, exts);
#else
  char *html_cstr = cmark_render_html(document, options);
#endif

  cmark_node_free(document);
  cmark_parser_free(parser);

  // Take ownership of cmark's malloc'd buffer
  html_owner_ = std::unique_ptr<char, decltype(&free)>(html_cstr, &free);

  // Point the view into the owned buffer
  html_view_ = std::string_view(html_owner_.get(), std::strlen(html_owner_.get()));

  return html_view_;
}
