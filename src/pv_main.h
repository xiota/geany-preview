/* -*- C++ -*-
 * Preview Geany Plugin
 * Copyright 2021 xiota
 *
 * Code Format, Markdown (Geany Plugins)
 * Copyright 2013 Matthew <mbrush@codebrainz.ca>
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

#include <locale>

#include "geanyplugin.h"
#include "webkit2/webkit2.h"

extern GeanyPlugin *geany_plugin;
extern GeanyData *geany_data;

enum PreviewFileType {
  PREVIEW_FILETYPE_NONE,
  PREVIEW_FILETYPE_ASCIIDOC,
  PREVIEW_FILETYPE_DOCBOOK,
  PREVIEW_FILETYPE_DOKUWIKI,
  PREVIEW_FILETYPE_EMAIL,
  PREVIEW_FILETYPE_FOUNTAIN,
  PREVIEW_FILETYPE_GFM,
  PREVIEW_FILETYPE_HTML,
  PREVIEW_FILETYPE_LATEX,
  PREVIEW_FILETYPE_MARKDOWN,
  PREVIEW_FILETYPE_MEDIAWIKI,
  PREVIEW_FILETYPE_MUSE,
  PREVIEW_FILETYPE_ORG,
  PREVIEW_FILETYPE_PLAIN,
  PREVIEW_FILETYPE_REST,
  PREVIEW_FILETYPE_TEXTILE,
  PREVIEW_FILETYPE_TIKIWIKI,
  PREVIEW_FILETYPE_TWIKI,
  PREVIEW_FILETYPE_TXT2TAGS,
  PREVIEW_FILETYPE_VIMWIKI,
  PREVIEW_FILETYPE_WIKI,

  PREVIEW_FILETYPE_COUNT
};

enum PreviewShortcuts {
  PREVIEW_KEY_TOGGLE_EDITOR,
  PREVIEW_KEY_EXPORT_HTML,

#ifdef ENABLE_EXPORT_PDF
  PREVIEW_KEY_EXPORT_PDF,
#endif  // ENABLE_EXPORT_PDF

  PREVIEW_KEY_COUNT,
};
