/*
 * Preview Geany Plugin
 * Copyright 2021 xiota
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef PREVIEW_FORMATS_H
#define PREVIEW_FORMATS_H

#include "process.h"

G_BEGIN_DECLS

extern GeanyPlugin *geany_plugin;
extern GeanyData *geany_data;

extern struct PreviewSettings settings;

enum PandocOptions {
  PANDOC_NONE = 0,
  PANDOC_FRAGMENT = 1,  // turn off --standalone
  PANDOC_TOC = 2
};

// wrappers around command line programs
GString *pandoc(const char *work_dir, const char *input, const char *in_format);
GString *asciidoctor(const char *work_dir, const char *input);
GString *screenplain(const char *work_dir, const char *input,
                     const char *out_format);

G_END_DECLS

#endif  // PREVIEW_FORMATS_H
