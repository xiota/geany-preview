/* -*- C++ -*-
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

#ifndef PREVIEW_FORMATS_H
#define PREVIEW_FORMATS_H

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm.h>

#include <string>

#include "plugin.h"
#include "process.h"

G_BEGIN_DECLS

extern struct PreviewSettings settings;

enum PandocOptions {
  PANDOC_NONE = 0,
  PANDOC_FRAGMENT = 1,  // turn off --standalone
  PANDOC_TOC = 2
};

std::string find_css(std::string const &css_fn);
std::string find_copy_css(std::string const &css_fn, std::string const &src);

// wrappers around command line programs
char *pandoc(char const *work_dir, char const *input, char const *in_format);
char *asciidoctor(char const *work_dir, char const *input);
char *screenplain(char const *work_dir, char const *input,
                  char const *out_format);

static void addMarkdownExtension(cmark_parser *parser,
                                 std::string const &extName);
std::string cmark_gfm(std::string const &input);

G_END_DECLS

#endif  // PREVIEW_FORMATS_H
