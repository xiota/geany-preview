// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_ftn2xml.h"

#include "renderers_html.h"  // Fountain::ftn2html

std::string_view ConverterFtn2xml::toHtml(std::string_view source) {
  html_ = Fountain::ftn2html(std::string(source), "fountain-html.css", false);
  return html_;
}
