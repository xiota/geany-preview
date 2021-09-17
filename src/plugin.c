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

#include <cmark-gfm.h>

#include "common.h"
#include "formats.h"
#include "plugin.h"
#include "prefs.h"
#include "process.h"

#define WEBVIEW_WARN(msg) \
  webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer), (msg))

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Preview",
                "Quick previews of HTML, Markdown, and other formats.", "0.1",
                "xiota")

// Function Declarations
static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data);
static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data);
static void on_document_filetype_set(GObject *obj, GeanyDocument *doc,
                                     GeanyFiletype *ft_old, gpointer user_data);

static void update_preview();

// Globals
static GtkWidget *g_viewer = NULL;
static GtkWidget *g_scrolled_win = NULL;
static gint g_page_num = 0;
static gint32 g_scrollY = 0;
static gulong g_load_handle = 0;
static guint g_timeout_handle = 0;

extern struct PreviewSettings settings;

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
  open_settings();

  WebKitSettings *wv_settings = webkit_settings_new();
  webkit_settings_set_default_font_family(wv_settings,
                                          settings.default_font_family);

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

  WEBVIEW_WARN("Loading...");

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
    WEBVIEW_WARN("Unknown document type.");
    return;
  }

  char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
  char *basename = g_path_get_basename(DOC_FILENAME(doc));
  char *work_dir = g_path_get_dirname(DOC_FILENAME(doc));
  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  char *html = NULL;
  char *plain = NULL;
  GString *output = NULL;

  // save scroll position
  wv_save_scroll_pos();

  // callback to restore scroll position
  if (g_load_handle == 0) {
    g_load_handle =
        g_signal_connect_swapped(WEBKIT_WEB_VIEW(g_viewer), "load-changed",
                                 G_CALLBACK(wv_loading_callback), NULL);
  }

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_HTML:
      if (REGEX_CHK("disable", settings.html_processor)) {
        plain = g_strdup("Preview of HTML documents has been disabled.");
      } else if (REGEX_CHK("pandoc", settings.html_processor)) {
        output = pandoc(work_dir, text, "html");
      } else {
        html = g_strdup(text);
      }
      break;
    case GEANY_FILETYPES_MARKDOWN:
      if (REGEX_CHK("disable", settings.markdown_processor)) {
        plain = g_strdup("Preview of Markdown documents has been disabled.");
      } else if (REGEX_CHK("pandoc", settings.markdown_processor)) {
        output = pandoc(work_dir, text, settings.pandoc_markdown);
      } else {
        html = cmark_markdown_to_html(text, strlen(text), 0);
      }
      break;
    case GEANY_FILETYPES_ASCIIDOC:
      if (REGEX_CHK("disable", settings.asciidoc_processor)) {
        plain = g_strdup("Preview of AsciiDoc documents has been disabled.");
      } else {
        output = asciidoctor(work_dir, text);
      }
      break;
    case GEANY_FILETYPES_DOCBOOK:
      output = pandoc(work_dir, text, "docbook");
      break;
    case GEANY_FILETYPES_LATEX:
      output = pandoc(work_dir, text, "latex");
      break;
    case GEANY_FILETYPES_REST:
      output = pandoc(work_dir, text, "rst");
      break;
    case GEANY_FILETYPES_TXT2TAGS:
      output = pandoc(work_dir, text, "t2t");
      break;

#define CHECK_TYPE(_type) \
  g_regex_match_simple((_type), basename, G_REGEX_CASELESS, 0)

    case GEANY_FILETYPES_NONE:
      if (!settings.extended_types) {
        plain = g_strdup("Extended file type detection has been disabled.");
        break;
      } else if (CHECK_TYPE("gfm")) {
        output = pandoc(work_dir, text, "gfm");
      } else if (CHECK_TYPE("fountain") || CHECK_TYPE("spmd")) {
        if (REGEX_CHK("disable", settings.fountain_processor)) {
          plain =
              g_strdup("Preview of Fountain screenplays has been disabled.");
        } else {
          output = screenplain(work_dir, text, "html");
        }
      } else if (CHECK_TYPE("textile")) {
        output = pandoc(work_dir, text, "textile");
      } else if (CHECK_TYPE("txt")) {
        if (CHECK_TYPE("gfm")) {
          output = pandoc(work_dir, text, "gfm");
        } else if (CHECK_TYPE("pandoc")) {
          output = pandoc(work_dir, text, "markdown");
        } else if (settings.verbatim_plain_text) {
          plain = g_strdup(text);
        } else {
          plain = g_strdup("Verbatim text has been disabled.");
        }
      } else if (CHECK_TYPE("wiki")) {
        if (REGEX_CHK("disable", settings.wiki_default)) {
          plain = g_strdup("Preview of wiki documents has been disabled.");
        } else if (CHECK_TYPE("dokuwiki")) {
          output = pandoc(work_dir, text, "dokuwiki");
        } else if (CHECK_TYPE("tikiwiki")) {
          output = pandoc(work_dir, text, "tikiwiki");
        } else if (CHECK_TYPE("vimwiki")) {
          output = pandoc(work_dir, text, "vimwiki");
        } else if (CHECK_TYPE("twiki")) {
          output = pandoc(work_dir, text, "twiki");
        } else if (CHECK_TYPE("mediawiki") || CHECK_TYPE("wikipedia")) {
          output = pandoc(work_dir, text, "mediawiki");
        } else {
          output = pandoc(work_dir, text, settings.wiki_default);
        }
      } else if (CHECK_TYPE("muse")) {
        output = pandoc(work_dir, text, "muse");
      } else if (CHECK_TYPE("org")) {
        output = pandoc(work_dir, text, "org");
      }
      break;
#undef CHECK_TYPE

    // case GEANY_FILETYPES_XML:
    default: {
      // html = NULL;
    } break;
  }

  if (html) {
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), html, uri);
    g_free(html);
  } else if (output) {
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), output->str, uri);
    g_string_free(output, TRUE);
  } else if (plain) {
    WEBVIEW_WARN(plain);
    g_free(plain);
  } else {
    plain = g_malloc(64);
    sprintf(plain, "Unable to process type: %s, %s.", doc->file_type->name,
            doc->encoding);
    WEBVIEW_WARN(plain);
    g_free(plain);
  }

  g_free(uri);
  g_free(basename);
  g_free(work_dir);

  // restore scroll position
  wv_load_scroll_pos();
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
    WEBVIEW_WARN("Unknown document type.");
    return FALSE;
  }
  if (notif->nmhdr.code == SCN_MODIFIED && notif->length > 0) {
    if (notif->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
      if (gtk_notebook_get_current_page(nb) != g_page_num &&
          g_timeout_handle == 0) {
        // delay updates when preview is not visible,
        // but still need to update in case user switches tabs
        // TODO: Stop updates entirely when not visible
        g_timeout_handle = g_timeout_add(settings.background_interval,
                                         update_preview_timeout_callback, NULL);
      } else if (doc->file_type->id != GEANY_FILETYPES_ASCIIDOC &&
                 doc->file_type->id != GEANY_FILETYPES_NONE) {
        // delay for fast programs
        int length = (int)scintilla_send_message(doc->editor->sci,
                                                 SCI_GETTEXTLENGTH, 0, 0);
        double _tt = (double)length * settings.size_factor_fast;
        int timeout = (int)_tt > settings.update_interval_fast
                          ? (int)_tt
                          : settings.update_interval_fast;

        g_timeout_handle =
            g_timeout_add(timeout, update_preview_timeout_callback, NULL);
      } else if (g_timeout_handle == 0) {
        // delay for slow external programs
        int length = (int)scintilla_send_message(doc->editor->sci,
                                                 SCI_GETTEXTLENGTH, 0, 0);
        double _tt = (double)length * settings.size_factor_slow;
        int timeout = (int)_tt > settings.update_interval_slow
                          ? (int)_tt
                          : settings.update_interval_slow;

        g_timeout_handle =
            g_timeout_add(timeout, update_preview_timeout_callback, NULL);
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
