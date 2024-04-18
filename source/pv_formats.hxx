/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm.h>

#include <string>

#include "process.hxx"
#include "pv_settings.hxx"
#include "tkui_addons.hxx"

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
