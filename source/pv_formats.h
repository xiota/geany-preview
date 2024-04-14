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

std::string find_css(std::string const &css_fn);
std::string find_copy_css(std::string const &css_fn, std::string const &src);

// wrappers around command line programs
std::string pandoc(std::string const &work_dir, std::string const &input,
                   std::string const &in_format);

std::string asciidoctor(std::string const &work_dir, std::string const &input);
std::string asciidoc(std::string const &work_dir, std::string const &input);

void addMarkdownExtension(cmark_parser *parser, std::string const &extName);
std::string cmark_gfm(std::string const &input);
