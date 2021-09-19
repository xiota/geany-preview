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

#include "formats.h"
#include "plugin.h"
#include "prefs.h"
#include "process.h"

#define WEBVIEW_WARN(msg) \
  webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer), (msg))

/* ********************
 * Declarations
 */
static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data);
static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data);
static void on_document_filetype_set(GObject *obj, GeanyDocument *doc,
                                     GeanyFiletype *ft_old, gpointer user_data);

static gboolean update_timeout_callback(gpointer user_data);
static void update_preview();

static void tab_switch_callback(GtkNotebook *nb);

/* ********************
 * Globals
 */
GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static gboolean preview_init(GeanyPlugin *plugin, gpointer data);
static void preview_cleanup(GeanyPlugin *plugin, gpointer data);

static GtkWidget *g_viewer = NULL;
static WebKitWebContext *g_wv_context = NULL;
static GtkWidget *g_scrolled_win = NULL;
static gint g_page_num = 0;
static gint32 g_scrollY = 0;
static gulong g_load_handle = 0;
static guint g_timeout_handle = 0;
static gboolean g_snippet = FALSE;
static gulong g_tab_handle = 0;

extern struct PreviewSettings settings;

/* ********************
 * Plugin Setup
 */
void geany_load_module(GeanyPlugin *plugin) {
  plugin->info->name = _("Preview");
  plugin->info->description =
      _("Quick previews of HTML, Markdown, and other formats.");
  plugin->info->version = "0.1";
  plugin->info->author = _("xiota");

  plugin->funcs->init = preview_init;
  plugin->funcs->configure = NULL;
  plugin->funcs->help = NULL;
  plugin->funcs->cleanup = preview_cleanup;

#define PREVIEW_PSC(sig, cb) \
  plugin_signal_connect(plugin, NULL, (sig), TRUE, G_CALLBACK(cb), NULL)

  PREVIEW_PSC("geany-startup-complete", on_document_signal);
  PREVIEW_PSC("editor-notify", on_editor_notify);
  PREVIEW_PSC("document-activate", on_document_signal);
  PREVIEW_PSC("document-filetype-set", on_document_filetype_set);
  PREVIEW_PSC("document-new", on_document_signal);
  PREVIEW_PSC("document-open", on_document_signal);
  PREVIEW_PSC("document-reload", on_document_signal);
#undef CONNECT

  GEANY_PLUGIN_REGISTER(plugin, 225);
  geany_plugin_set_data(plugin, plugin, NULL);
}

static gboolean preview_init(GeanyPlugin *plugin, gpointer data) {
  geany_plugin = plugin;
  geany_data = plugin->geany_data;

  open_settings();

  WebKitSettings *wv_settings = webkit_settings_new();
  webkit_settings_set_default_font_family(wv_settings,
                                          settings.default_font_family);

  g_viewer = webkit_web_view_new_with_settings(wv_settings);
  g_wv_context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(g_viewer));

  g_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(g_scrolled_win), g_viewer);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_scrolled_win),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  g_page_num =
      gtk_notebook_append_page(nb, g_scrolled_win, gtk_label_new("Preview"));

  gtk_widget_show_all(g_scrolled_win);
  gtk_notebook_set_current_page(nb, g_page_num);

  // signal handler to update when notebook selected
  g_tab_handle = g_signal_connect(GTK_WIDGET(nb), "switch_page",
                                  G_CALLBACK(tab_switch_callback), NULL);

  WEBVIEW_WARN("Loading.");

  // preview may need to be updated after a delay on first use
  if (g_timeout_handle == 0) {
    g_timeout_handle = g_timeout_add(settings.background_interval / 2,
                                     update_timeout_callback, NULL);
  }
  return TRUE;
}

static void preview_cleanup(GeanyPlugin *plugin, gpointer data) {
  // fmt_prefs_deinit();
  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  gtk_notebook_remove_page(nb, g_page_num);

  gtk_widget_destroy(g_viewer);
  gtk_widget_destroy(g_scrolled_win);

  g_clear_signal_handler(&g_tab_handle, GTK_WIDGET(nb));
}

/* ********************
 * Functions
 */
static void tab_switch_callback(GtkNotebook *nb) {
  // GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  if (gtk_notebook_get_current_page(nb) == g_page_num) {
    if (g_timeout_handle == 0) {
      g_timeout_handle = g_timeout_add(settings.update_interval_fast,
                                       update_timeout_callback, NULL);
    }
  }
}

static void wv_save_position_callback(GObject *object, GAsyncResult *result,
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

static void wv_save_position() {
  char *script;
  script = g_strdup("window.scrollY");

  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, NULL,
                                 wv_save_position_callback, NULL);
  g_free(script);
}

static void wv_load_position() {
  char *script;
  if (g_snippet) {
    script = g_strdup(
        "window.scrollTo(0, 0.2*document.documentElement.scrollHeight);");
  } else {
    script = g_strdup_printf("window.scrollTo(0, %d);", g_scrollY);
  }
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, NULL, NULL,
                                 NULL);
  g_free(script);
}

static void wv_loading_callback(WebKitWebView *web_view,
                                WebKitLoadEvent load_event,
                                gpointer user_data) {
  // don't wait for WEBKIT_LOAD_FINISHED, othewise preview will flicker
  wv_load_position();
}

static void update_preview() {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN("Unknown document type.");
    return;
  }

  // save scroll position and set callback if needed
  wv_save_position();
  if (g_load_handle == 0) {
    g_load_handle =
        g_signal_connect_swapped(WEBKIT_WEB_VIEW(g_viewer), "load-changed",
                                 G_CALLBACK(wv_loading_callback), NULL);
  }

  char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
  char *basename = g_path_get_basename(DOC_FILENAME(doc));
  char *work_dir = g_path_get_dirname(DOC_FILENAME(doc));
  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  char *html = NULL;
  char *plain = NULL;
  GString *output = NULL;

  int position = 0;
  int line = sci_get_current_line(doc->editor->sci);

  int length = sci_get_length(doc->editor->sci);
  if (g_snippet) {
    if (line > 0) {
      position = sci_get_position_from_line(doc->editor->sci, line);
    }
    int start = 0;
    int end = 0;
    int amount = settings.snippet_window / 3;
    // get beginning and end and set scroll position
    if (position > amount) {
      start = position - amount;
    } else {
      start = 0;
    }
    if (position + 2 * amount > length) {
      end = length;
    } else {
      end = position + 2 * amount;
    }

    text = sci_get_contents_range(doc->editor->sci, start, end);
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
    case GEANY_FILETYPES_NONE:
      if (!settings.extended_types) {
        plain = g_strdup("Extended file type detection has been disabled.");
        break;
      } else if (REGEX_CHK("gfm", basename)) {
        output = pandoc(work_dir, text, "gfm");
      } else if (REGEX_CHK("fountain", basename) ||
                 REGEX_CHK("spmd", basename)) {
        if (REGEX_CHK("disable", settings.fountain_processor)) {
          plain =
              g_strdup("Preview of Fountain screenplays has been disabled.");
        } else {
          output = screenplain(work_dir, text, "html");
        }
      } else if (REGEX_CHK("textile", basename)) {
        output = pandoc(work_dir, text, "textile");
      } else if (REGEX_CHK("txt", basename)) {
        if (REGEX_CHK("gfm", basename)) {
          output = pandoc(work_dir, text, "gfm");
        } else if (REGEX_CHK("pandoc", basename)) {
          output = pandoc(work_dir, text, "markdown");
        } else if (settings.verbatim_plain_text) {
          plain = g_strdup(text);
        } else {
          plain = g_strdup("Verbatim text has been disabled.");
        }
      } else if (REGEX_CHK("wiki", basename)) {
        if (REGEX_CHK("disable", settings.wiki_default)) {
          plain = g_strdup("Preview of wiki documents has been disabled.");
        } else if (REGEX_CHK("dokuwiki", basename)) {
          output = pandoc(work_dir, text, "dokuwiki");
        } else if (REGEX_CHK("tikiwiki", basename)) {
          output = pandoc(work_dir, text, "tikiwiki");
        } else if (REGEX_CHK("vimwiki", basename)) {
          output = pandoc(work_dir, text, "vimwiki");
        } else if (REGEX_CHK("twiki", basename)) {
          output = pandoc(work_dir, text, "twiki");
        } else if (REGEX_CHK("mediawiki", basename) ||
                   REGEX_CHK("wikipedia", basename)) {
          output = pandoc(work_dir, text, "mediawiki");
        } else {
          output = pandoc(work_dir, text, settings.wiki_default);
        }
      } else if (REGEX_CHK("muse", basename)) {
        output = pandoc(work_dir, text, "muse");
      } else if (REGEX_CHK("org", basename)) {
        output = pandoc(work_dir, text, "org");
      }
      break;
    // case GEANY_FILETYPES_XML:
    default:
      break;
  }

  if (g_snippet) {
    g_free(text);
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
  wv_load_position();
}

static gboolean update_timeout_callback(gpointer user_data) {
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

  int length = sci_get_length(doc->editor->sci);
  if (length > settings.snippet_trigger) {
    g_snippet = TRUE;
  } else {
    g_snippet = FALSE;
  }

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_ASCIIDOC:
      if (!settings.snippet_asciidoctor) {
        g_snippet = FALSE;
      }
      break;
    case GEANY_FILETYPES_HTML:
      if (!settings.snippet_html) {
        g_snippet = FALSE;
      }
      break;
    case GEANY_FILETYPES_LATEX:
    case GEANY_FILETYPES_MARKDOWN:
      if (!settings.snippet_markdown) {
        g_snippet = FALSE;
      }
      break;
    case GEANY_FILETYPES_NONE: {
      char *basename = g_path_get_basename(DOC_FILENAME(doc));
      if (REGEX_CHK("fountain", basename) || REGEX_CHK("spmd", basename)) {
        if (!settings.snippet_screenplain) {
          g_snippet = FALSE;
        }
      }
    }  // no break; need to check pandoc
    case GEANY_FILETYPES_DOCBOOK:
    case GEANY_FILETYPES_REST:
    case GEANY_FILETYPES_TXT2TAGS:
    default:
      if (!settings.snippet_pandoc) {
        g_snippet = FALSE;
      }
      break;
  }

  gboolean need_update = FALSE;
  if (notif->nmhdr.code == SCN_UPDATEUI && g_snippet &&
      (notif->updated & (SC_UPDATE_CONTENT | SC_UPDATE_SELECTION))) {
    need_update = TRUE;
  }

  if (notif->nmhdr.code == SCN_MODIFIED && notif->length > 0) {
    if (notif->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      need_update = TRUE;
    }
  }

  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  if (gtk_notebook_get_current_page(nb) != g_page_num ||
      !gtk_widget_is_visible(GTK_WIDGET(nb))) {
    // no updates when preview pane is hidden
    return FALSE;
  }

  if (need_update && g_timeout_handle == 0) {
    if (doc->file_type->id != GEANY_FILETYPES_ASCIIDOC &&
        doc->file_type->id != GEANY_FILETYPES_NONE) {
      // delay for faster programs
      double _tt = (double)length * settings.size_factor_fast;
      int timeout = (int)_tt > settings.update_interval_fast
                        ? (int)_tt
                        : settings.update_interval_fast;
      g_timeout_handle = g_timeout_add(timeout, update_timeout_callback, NULL);
    } else {
      // delay for slower programs
      double _tt = (double)length * settings.size_factor_slow;
      int timeout = (int)_tt > settings.update_interval_slow
                        ? (int)_tt
                        : settings.update_interval_slow;
      g_timeout_handle = g_timeout_add(timeout, update_timeout_callback, NULL);
    }
  }

  return FALSE;
}

static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data) {
  webkit_web_context_clear_cache(g_wv_context);
  update_preview();
}

static void on_document_filetype_set(GObject *obj, GeanyDocument *doc,
                                     GeanyFiletype *ft_old,
                                     gpointer user_data) {
  webkit_web_context_clear_cache(g_wv_context);
  update_preview();
}
