/*
 * plugin.h
 *
 * Copyright 2013 Matthew <mbrush@codebrainz.ca>
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

#ifndef PREVIEW_PLUGIN_H
#define PREVIEW_PLUGIN_H

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

//#include <Scintilla.h>
//#include <ScintillaWidget.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#endif
#include <geanyplugin.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

G_BEGIN_DECLS

extern GeanyPlugin *geany_plugin;
extern GeanyData *geany_data;

enum PreviewFileType {
  NONE,
  ASCIIDOC,
  DOCBOOK,
  DOKUWIKI,
  EMAIL,
  FOUNTAIN,
  GFM,
  HTML,
  LATEX,
  MARKDOWN,
  MEDIAWIKI,
  MUSE,
  ORG,
  PLAIN,
  REST,
  TEXTILE,
  TIKIWIKI,
  TWIKI,
  TXT2TAGS,
  VIMWIKI,
  WIKI
};

enum PreviewShortcuts {
  PREVIEW_KEY_TOGGLE_EDITOR,
};

#define GEANY_PSC(sig, cb) \
  plugin_signal_connect(geany_plugin, NULL, (sig), TRUE, G_CALLBACK(cb), NULL)

#define GFREE(_z_) \
  do {             \
    g_free(_z_);   \
    _z_ = NULL;    \
  } while (0)

#define GSTRING_FREE(_z_)     \
  do {                        \
    g_string_free(_z_, TRUE); \
    _z_ = NULL;               \
  } while (0)

#define GERROR_FREE(_z_) \
  do {                   \
    g_error_free(_z_);   \
    _z_ = NULL;          \
  } while (0)

#define GKEY_FILE_FREE(_z_) \
  do {                      \
    g_key_file_free(_z_);   \
    _z_ = NULL;             \
  } while (0)

#define REGEX_CHK(_tp, _str) \
  g_regex_match_simple((_tp), (_str), G_REGEX_CASELESS, 0)

G_END_DECLS

#endif  // PREVIEW_PLUGIN_H
