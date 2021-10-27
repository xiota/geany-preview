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

#ifndef PREVIEW_PLUGIN_H
#define PREVIEW_PLUGIN_H

#include <geanyplugin.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <string>

G_BEGIN_DECLS

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

#define PREVIEW_FILETYPE_NOEXIST PREVIEW_FILETYPE_COUNT

#ifndef GEANY_FILETYPES_FOUNTAIN
#define GEANY_FILETYPES_FOUNTAIN PREVIEW_FILETYPE_NOEXIST
#endif

enum PreviewShortcuts {
  PREVIEW_KEY_TOGGLE_EDITOR,
  PREVIEW_KEY_EXPORT_HTML,
  PREVIEW_KEY_EXPORT_PDF,

  PREVIEW_KEY_COUNT
};

static bool preview_init(GeanyPlugin *plugin, gpointer data);
static void preview_cleanup(GeanyPlugin *plugin, gpointer data);
static GtkWidget *preview_configure(GeanyPlugin *plugin, GtkDialog *dialog,
                                    gpointer pdata);

static bool on_editor_notify(GObject *obj, GeanyEditor *editor,
                             SCNotification *notif, gpointer user_data);
static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data);

static void on_pref_open_config_folder(GtkWidget *self, GtkWidget *dialog);
static void on_pref_edit_config(GtkWidget *self, GtkWidget *dialog);
static void on_pref_reload_config(GtkWidget *self, GtkWidget *dialog);
static void on_pref_save_config(GtkWidget *self, GtkWidget *dialog);
static void on_pref_reset_config(GtkWidget *self, GtkWidget *dialog);

static GtkWidget *find_focus_widget(GtkWidget *widget);
static void on_toggle_editor_preview();
static bool on_key_binding(int key_id);
static bool on_sidebar_focus(GtkWidget *widget, GtkDirectionType direction,
                             gpointer user_data);
static bool on_sidebar_focus_in(GtkWidget *widget, GdkEvent *event,
                                gpointer user_data);
static bool on_sidebar_focus_out(GtkWidget *widget, GdkEvent *event,
                                 gpointer user_data);

static void on_pref_open_config_folder(GtkWidget *self, GtkWidget *dialog);

static void on_menu_preferences(GtkWidget *self, GtkWidget *dialog);
static void on_menu_export_html(GtkWidget *self, GtkWidget *dialog);
static void on_menu_export_pdf(GtkWidget *self, GtkWidget *dialog);
static std::string replace_extension(std::string const &fn,
                                     std::string const &ext);

static gboolean update_timeout_callback(gpointer user_data);
static std::string update_preview(bool const bGetContents = false);

static void on_sidebar_switch_page(GtkNotebook *nb, GtkWidget *page,
                                   uint page_num, gpointer user_data);
static void on_sidebar_show(GtkNotebook *nb, gpointer user_data);
static void on_sidebar_state_flags_changed(GtkWidget *widget,
                                           GtkStateFlags flags,
                                           gpointer user_data);

static PreviewFileType get_filetype_from_filename(std::string const &fn);
static PreviewFileType get_filetype(GeanyDocument *doc);
static bool use_snippets(GeanyDocument *doc);
static void wv_apply_settings();

G_END_DECLS

#define GEANY_PSC(sig, cb)                                                  \
  plugin_signal_connect(geany_plugin, nullptr, (sig), true, G_CALLBACK(cb), \
                        nullptr)

#define GFREE(_z_) \
  do {             \
    g_free(_z_);   \
    _z_ = nullptr; \
  } while (0)

#define GSTRING_FREE(_z_)     \
  do {                        \
    g_string_free(_z_, true); \
    _z_ = nullptr;            \
  } while (0)

#define GERROR_FREE(_z_) \
  do {                   \
    g_error_free(_z_);   \
    _z_ = nullptr;       \
  } while (0)

#define GKEY_FILE_FREE(_z_) \
  do {                      \
    g_key_file_free(_z_);   \
    _z_ = nullptr;          \
  } while (0)

#define SUBSTR(needle, haystack) (strstr((haystack), (needle)) != nullptr)

#ifndef g_clear_signal_handler
#include "gobject/gsignal.h"
// g_clear_signal_handler was added in glib 2.62
#define g_clear_signal_handler(handler, instance)      \
  do {                                                 \
    if (handler != nullptr && *handler != 0) {         \
      g_signal_handler_disconnect(instance, *handler); \
      *handler = 0;                                    \
    }                                                  \
  } while (0)
#endif  // g_clear_signal_handler

#endif  // PREVIEW_PLUGIN_H
