/*
 * Preview Geany Plugin
 * Copyright 2021 xiota
 *
 * Code Format, Markdown Geany Plugins
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "plugin.h"
#include "process.h"

#ifndef _
#define _(s) s
#endif

#include <cmark-gfm.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO(_("Preview"),
                _("Rendered preview of HTML, Markdown, and other documents."),
                _("0.1"), _("xiota"))

// Function Declarations
static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data);
static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data);
static void on_document_filetype_set(GObject *obj, GeanyDocument *doc,
                                     GeanyFiletype *ft_old, gpointer user_data);

GString *pandoc(const char *work_dir, const char *input, char *type);
GString *asciidoctor(const char *work_dir, const char *input);

static void update_preview();

// Globals
static GtkWidget *g_viewer = NULL;
static GtkWidget *g_scrolled_win = NULL;
static gint g_page_num = 0;
static gint32 g_scrollY = 0;
static gulong g_load_handle = 0;
static guint g_timeout_handle = 0;

// Functions

static void wv_load_scroll_callback(GObject *object, GAsyncResult *result,
                                    gpointer user_data) {}

static void wv_save_scroll_pos_callback(GObject *object, GAsyncResult *result,
                                        gpointer user_data) {
  WebKitJavascriptResult *js_result;
  JSCValue *value;
  GError *error = NULL;

  js_result = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(object),
                                                    result, &error);
  if (!js_result) {
    g_warning("Error running javascript: %s", error->message);
    g_error_free(error);
    return;
  }

  value = webkit_javascript_result_get_js_value(js_result);
  gint32 temp = jsc_value_to_int32(value);
  if (temp > 0) {
    g_scrollY = temp;
  }

  webkit_javascript_result_unref(js_result);
}

static void wv_save_scroll_pos() {
  char *script;
  script = g_strdup("window.scrollY");

  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, NULL,
                                 wv_save_scroll_pos_callback, NULL);
  g_free(script);
}

static void wv_load_scroll_pos() {
  char *script;
  script = g_strdup_printf("window.scrollTo(0, %d);", g_scrollY);

  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, NULL,
                                 wv_load_scroll_callback, NULL);
  g_free(script);
}

static void wv_loading_callback(WebKitWebView *web_view,
                                WebKitLoadEvent load_event,
                                gpointer user_data) {
  // don't wait for WEBKIT_LOAD_FINISHED, othewise preview will flicker
  wv_load_scroll_pos();
}

void plugin_init(G_GNUC_UNUSED GeanyData *data) {
  WebKitSettings *wv_settings = webkit_settings_new();
  webkit_settings_set_enable_java(wv_settings, FALSE);
  webkit_settings_set_enable_javascript(wv_settings, TRUE);
  webkit_settings_set_enable_javascript_markup(wv_settings, FALSE);
  webkit_settings_set_allow_file_access_from_file_urls(wv_settings, TRUE);
  webkit_settings_set_allow_universal_access_from_file_urls(wv_settings, TRUE);

  webkit_settings_set_default_font_family(wv_settings, "serif");
  // webkit_settings_set_monospace_font_family(wv_settings, "monospace");
  // webkit_settings_set_serif_font_family (wv_settings, "serif");
  // webkit_settings_set_sans_serif_font_family(wv_settings, "sans-serif");
  // webkit_settings_set_cursive_font_family(wv_settings, "cursive");
  // webkit_settings_set_fantasy_font_family(wv_settings, "fantasy");
  // webkit_settings_set_pictograph_font_family(wv_settings, "pictograph");

  g_viewer = webkit_web_view_new_with_settings(wv_settings);

  g_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(g_scrolled_win), g_viewer);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_scrolled_win),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  g_page_num =
      gtk_notebook_append_page(nb, g_scrolled_win, gtk_label_new("Preview"));

  gtk_widget_show_all(g_scrolled_win);
  gtk_notebook_set_current_page(nb, g_page_num);

  webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer), "Loading...");

#define CONNECT(sig, cb) \
  plugin_signal_connect(geany_plugin, NULL, sig, TRUE, G_CALLBACK(cb), NULL)

  CONNECT("geany-startup-complete", on_document_signal);
  CONNECT("editor-notify", on_editor_notify);
  CONNECT("document-activate", on_document_signal);
  CONNECT("document-filetype-set", on_document_filetype_set);
  CONNECT("document-new", on_document_signal);
  CONNECT("document-open", on_document_signal);
  CONNECT("document-reload", on_document_signal);
#undef CONNECT
}

void plugin_cleanup() {
  // fmt_prefs_deinit();
  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  gtk_notebook_remove_page(nb, g_page_num);

  gtk_widget_destroy(g_viewer);
  gtk_widget_destroy(g_scrolled_win);
}

static void update_preview() {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer),
                                    "Unknown document type.");
    return;
  }

  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  wv_save_scroll_pos();

  if (g_load_handle == 0) {
    g_load_handle =
        g_signal_connect_swapped(WEBKIT_WEB_VIEW(g_viewer), "load-changed",
                                 G_CALLBACK(wv_loading_callback), NULL);
  }

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_MARKDOWN: {
      // TODO: Add option to switch markdown interpreter
      char *html = cmark_markdown_to_html(text, strlen(text), 0);
      char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
      webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html, uri);
      g_free(html);
      g_free(uri);
    } break;

    case GEANY_FILETYPES_HTML: {
      char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
      webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), text, uri);
      g_free(uri);
    } break;

    case GEANY_FILETYPES_ASCIIDOC: {
      GString *html = asciidoctor(g_path_get_dirname(DOC_FILENAME(doc)), text);
      if (html) {
        char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html->str, uri);
        g_string_free(html, TRUE);
        g_free(uri);
      }
    } break;

    case GEANY_FILETYPES_DOCBOOK: {
      GString *html =
          pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "docbook");
      if (html) {
        char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html->str, uri);
        g_string_free(html, TRUE);
        g_free(uri);
      }
    } break;

    case GEANY_FILETYPES_LATEX: {
      GString *html =
          pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "latex");
      if (html) {
        char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html->str, uri);
        g_string_free(html, TRUE);
        g_free(uri);
      }
    } break;

    case GEANY_FILETYPES_REST: {
      GString *html =
          pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "rst");
      if (html) {
        char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html->str, uri);
        g_string_free(html, TRUE);
        g_free(uri);
      }
    } break;

    case GEANY_FILETYPES_TXT2TAGS: {
      GString *html =
          pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "t2t");
      if (html) {
        char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html->str, uri);
        g_string_free(html, TRUE);
        g_free(uri);
      }
    } break;

    // case GEANY_FILETYPES_XML:
    case GEANY_FILETYPES_NONE: {
      char *filename = g_path_get_basename(DOC_FILENAME(doc));
      int _len = strlen(filename);
      GString *html = NULL;
      if (g_strcmp0(&filename[_len - 8], ".textile") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "textile");
      } else if (g_strcmp0(&filename[_len - 9], ".dokuwiki") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "dokuwiki");
      } else if (g_strcmp0(&filename[_len - 10], ".mediawiki") == 0 ||
                 g_strcmp0(&filename[_len - 5], ".wiki") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "mediawiki");
      } else if (g_strcmp0(&filename[_len - 5], ".muse") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "muse");
      } else if (g_strcmp0(&filename[_len - 9], ".tikiwiki") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "tikiwiki");
      } else if (g_strcmp0(&filename[_len - 8], ".vimwiki") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "vimwiki");
      } else if (g_strcmp0(&filename[_len - 6], ".twiki") == 0) {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, "twiki");
      } else {
        html = pandoc(g_path_get_dirname(DOC_FILENAME(doc)), text, NULL);
      }
      if (html) {
        char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html->str, uri);
        g_string_free(html, TRUE);
        g_free(uri);
      }
    } break;

    default: {
      char *_text = g_malloc(64);
      sprintf(_text, "Cannot preview type: %s, %s.", doc->file_type->name,
              doc->encoding);
      webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer), _text);
      g_free(_text);
    } break;
  }

  wv_load_scroll_pos();
}

GString *pandoc(const char *work_dir, const char *input, char *type) {
  if (input == NULL) {
    return NULL;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("pandoc"));

  if (type != NULL) {
    g_ptr_array_add(args, g_strdup_printf("--from=%s", type));
  }
  g_ptr_array_add(args, g_strdup("--to=html"));
  g_ptr_array_add(args, g_strdup("--quiet"));
  g_ptr_array_add(args, NULL);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);
  g_ptr_array_free(args, TRUE);

  if (!proc) {
    // command not found
    return NULL;
  }

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning("Failed to format document range");
    g_string_free(output, TRUE);
    fmt_process_close(proc);
    return NULL;
  }

  return output;
}

GString *asciidoctor(const char *work_dir, const char *input) {
  if (input == NULL) {
    return NULL;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("asciidoctor"));

  if (work_dir != NULL) {
    g_ptr_array_add(args, g_strdup_printf("--base-dir=DIR%s", work_dir));
  }
  g_ptr_array_add(args, g_strdup("--quiet"));
  g_ptr_array_add(args, g_strdup("--out-file=-"));
  g_ptr_array_add(args, g_strdup("-"));
  g_ptr_array_add(args, NULL);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);
  g_ptr_array_free(args, TRUE);

  if (!proc) {
    // command not found
    return NULL;
  }

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning("Failed to format document range");
    g_string_free(output, TRUE);
    fmt_process_close(proc);
    return NULL;
  }

  return output;
}

static gboolean update_preview_timeout_callback(gpointer user_data) {
  update_preview();
  g_timeout_handle = 0;
  return FALSE;
}

static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer),
                                    "Unknown document type.");
    return FALSE;
  }
  if (notif->nmhdr.code == SCN_MODIFIED && notif->length > 0) {
    if (notif->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
      if (gtk_notebook_get_current_page(nb) != g_page_num &&
          g_timeout_handle == 0) {
        // delay updates when preview is not visible
        g_timeout_handle =
            g_timeout_add(5000, update_preview_timeout_callback, NULL);
      } else if (doc->file_type->id != GEANY_FILETYPES_ASCIIDOC &&
                 doc->file_type->id != GEANY_FILETYPES_NONE) {
        update_preview();
      } else if (g_timeout_handle == 0) {
        g_timeout_handle =
            g_timeout_add(200, update_preview_timeout_callback, NULL);
      }
    }
  }
  return FALSE;
}

static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data) {
  update_preview();
}

static void on_document_filetype_set(GObject *obj, GeanyDocument *doc,
                                     GeanyFiletype *ft_old,
                                     gpointer user_data) {
  update_preview();
}
