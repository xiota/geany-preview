/*
 * Preview Geany Plugin
 * Copyright 2021 xiota
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm.h>

#include <string>

#include "process.h"
#include "pv_settings.h"
#include "tkui_addons.h"

enum PandocOptions {
  PANDOC_NONE = 0,
  PANDOC_FRAGMENT = 1,  // turn off --standalone
  PANDOC_TOC = 2
};

std::string find_css(const std::string &css_fn);
std::string find_copy_css(const std::string &css_fn, const std::string &src);

// wrappers around command line programs
std::string pandoc(const std::string &work_dir, const std::string &input,
                   const std::string &in_format);

std::string asciidoctor(const std::string &work_dir, const std::string &input);
std::string asciidoc(const std::string &work_dir, const std::string &input);

void addMarkdownExtension(cmark_parser *parser, const std::string &extName);
std::string cmark_gfm(const std::string &input);
