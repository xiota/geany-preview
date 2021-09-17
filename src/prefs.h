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

#ifndef PREVIEW_PREFS_H
#define PREVIEW_PREFS_H

#include <geanyplugin.h>
#include <gtk/gtk.h>

struct PreviewSettings {
  int update_interval;
  int background_interval;
  char *html_processor;
  char *markdown_processor;
  char *asciidoc_processor;
  char *fountain_processor;
  char *wiki_default;
  gboolean pandoc_fragment;
  gboolean pandoc_toc;
};

void init_settings();
void open_settings();
void load_settings(GKeyFile *kf);
void save_settings();

#endif  // PREVIEW_PREFS_H
