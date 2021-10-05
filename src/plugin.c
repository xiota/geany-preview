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
static gboolean preview_init(GeanyPlugin *plugin, gpointer data);
static void preview_cleanup(GeanyPlugin *plugin, gpointer data);
static GtkWidget *preview_configure(GeanyPlugin *plugin, GtkDialog *dialog,
                                    gpointer pdata);

static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data);
static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data);

static void on_pref_open_config_folder(GtkWidget *self, GtkWidget *dialog);
static void on_pref_edit_config(GtkWidget *self, GtkWidget *dialog);
static void on_pref_reload_config(GtkWidget *self, GtkWidget *dialog);
static void on_pref_save_config(GtkWidget *self, GtkWidget *dialog);
static void on_pref_reset_config(GtkWidget *self, GtkWidget *dialog);

static void on_menu_preferences(GtkWidget *self, GtkWidget *dialog);

static gboolean update_timeout_callback(gpointer user_data);
static char *update_preview(const gboolean get_contents);

static void tab_switch_callback(GtkNotebook *nb);
static inline enum PreviewFileType get_filetype(char *format);
static inline void set_filetype();
static inline void set_snippets();
static void wv_apply_settings();

/* ********************
 * Globals
 */
GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GtkWidget *g_preview_menu;
static GtkWidget *g_viewer = NULL;
static WebKitSettings *g_wv_settings = NULL;
static WebKitWebContext *g_wv_context = NULL;
WebKitUserContentManager *g_wv_content_manager = NULL;
static GtkWidget *g_scrolled_win = NULL;
static gint g_nb_page_num = 0;
static GArray *g_scrollY = NULL;
// static gint32 g_scrollY = 0;
static gulong g_load_handle = 0;
static guint g_timeout_handle = 0;
static gboolean g_snippet = FALSE;
static gulong g_tab_handle = 0;
static GeanyDocument *g_current_doc = NULL;

extern struct PreviewSettings settings;
enum PreviewFileType g_filetype = NONE;

/* ********************
 * Plugin Setup
 */
PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Preview",
                "Quick previews of HTML, Markdown, and other formats.", "0.1",
                "xiota")

void plugin_init(G_GNUC_UNUSED GeanyData *data) {
#define PREVIEW_PSC(sig, cb) \
  plugin_signal_connect(geany_plugin, NULL, (sig), TRUE, G_CALLBACK(cb), NULL)

  PREVIEW_PSC("geany-startup-complete", on_document_signal);
  PREVIEW_PSC("editor-notify", on_editor_notify);
  PREVIEW_PSC("document-activate", on_document_signal);
  PREVIEW_PSC("document-filetype-set", on_document_signal);
  PREVIEW_PSC("document-new", on_document_signal);
  PREVIEW_PSC("document-open", on_document_signal);
  PREVIEW_PSC("document-reload", on_document_signal);
#undef CONNECT

  preview_init(geany_plugin, geany_data);
}

void plugin_cleanup(void) { preview_cleanup(geany_plugin, geany_data); }

GtkWidget *plugin_configure(GtkDialog *dlg) {
  return preview_configure(geany_plugin, dlg, geany_data);
}

// void plugin_help(void) { }

static gboolean preview_init(GeanyPlugin *plugin, gpointer data) {
  geany_plugin = plugin;
  geany_data = plugin->geany_data;
  g_scrollY = g_array_new(TRUE, TRUE, sizeof(gint32));

  open_settings();

  // set up menu
  GeanyKeyGroup *group;
  GtkWidget *item;

  g_preview_menu = gtk_menu_item_new_with_label("Preview");
  ui_add_document_sensitive(g_preview_menu);

  GtkWidget *submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(g_preview_menu), submenu);

  item = gtk_menu_item_new_with_label("Edit Config File");
  g_signal_connect(item, "activate", G_CALLBACK(on_pref_edit_config), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label("Open Config Folder");
  g_signal_connect(item, "activate", G_CALLBACK(on_pref_open_config_folder),
                   NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label("Preferences");
  g_signal_connect(item, "activate", G_CALLBACK(on_menu_preferences), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  gtk_widget_show_all(g_preview_menu);

  gtk_menu_shell_append(GTK_MENU_SHELL(geany_data->main_widgets->tools_menu),
                        g_preview_menu);

  // set up webview
  g_wv_settings = webkit_settings_new();
  g_viewer = webkit_web_view_new_with_settings(g_wv_settings);

  g_wv_context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(g_viewer));
  g_wv_content_manager =
      webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(g_viewer));

  g_scrolled_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(g_scrolled_win), g_viewer);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_scrolled_win),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  g_nb_page_num =
      gtk_notebook_append_page(nb, g_scrolled_win, gtk_label_new("Preview"));

  gtk_widget_show_all(g_scrolled_win);
  gtk_notebook_set_current_page(nb, g_nb_page_num);

  // signal handler to update when notebook selected
  g_tab_handle = g_signal_connect(GTK_WIDGET(nb), "switch_page",
                                  G_CALLBACK(tab_switch_callback), NULL);

  wv_apply_settings();

  WEBVIEW_WARN("Loading.");

  // preview may need to be updated after a delay on first use
  if (g_timeout_handle == 0) {
    g_timeout_handle = g_timeout_add(2000, update_timeout_callback, NULL);
  }
  return TRUE;
}

static void preview_cleanup(GeanyPlugin *plugin, gpointer data) {
  // fmt_prefs_deinit();
  GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  gtk_notebook_remove_page(nb, g_nb_page_num);

  gtk_widget_destroy(g_viewer);
  gtk_widget_destroy(g_scrolled_win);

  gtk_widget_destroy(g_preview_menu);

  g_clear_signal_handler(&g_tab_handle, GTK_WIDGET(nb));
}

static void on_pref_open_config_folder(GtkWidget *self, GtkWidget *dialog) {
  char *conf_dn =
      g_build_filename(geany_data->app->configdir, "plugins", "preview", NULL);

  char *command;
  command = g_strdup_printf("xdg-open \"%s\"", conf_dn);
  if (system(command)) {
    // ignore;
  }
  GFREE(conf_dn);
  GFREE(command);
}

static void on_pref_edit_config(GtkWidget *self, GtkWidget *dialog) {
  open_settings();
  char *conf_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", NULL);
  GeanyDocument *doc = document_open_file(conf_fn, FALSE, NULL, NULL);
  document_reload_force(doc, NULL);
  GFREE(conf_fn);

  if (dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }
}

static void on_pref_reload_config(GtkWidget *self, GtkWidget *dialog) {
  open_settings();
  wv_apply_settings();
}

static void on_pref_save_config(GtkWidget *self, GtkWidget *dialog) {
  save_settings();
}

static void on_pref_reset_config(GtkWidget *self, GtkWidget *dialog) {
  save_default_settings();
}

static void on_menu_preferences(GtkWidget *self, GtkWidget *dialog) {
  plugin_show_configure(geany_plugin);
}

static GtkWidget *preview_configure(GeanyPlugin *plugin, GtkDialog *dialog,
                                    gpointer pdata) {
  GtkWidget *box, *btn;
  char *tooltip;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

  tooltip = g_strdup("Save the active settings to the config file.");
  btn = gtk_button_new_with_label("Save Config");
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_save_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip = g_strdup(
      "Reload settings from the config file.  May be used "
      "to apply preferences after editing without restarting Geany.");
  btn = gtk_button_new_with_label("Reload Config");
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_reload_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip = g_strdup(
      "Delete the current config file and restore the default "
      "file with explanatory comments.");
  btn = gtk_button_new_with_label("Reset Config");
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_reset_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip = g_strdup("Open the config file in Geany for editing.");
  btn = gtk_button_new_with_label("Edit Config");
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_edit_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip = g_strdup(
      "Open the config folder in the default file manager.  The config folder "
      "contains the stylesheets, which may be edited.");
  btn = gtk_button_new_with_label("Open Config Folder");
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_open_config_folder),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  return box;
}

/* ********************
 * Functions
 */
static void tab_switch_callback(GtkNotebook *nb) {
  // GtkNotebook *nb = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  if (gtk_notebook_get_current_page(nb) == g_nb_page_num) {
    if (g_timeout_handle == 0) {
      g_timeout_handle = g_timeout_add(settings.update_interval_fast,
                                       update_timeout_callback, NULL);
    }
  }
}

static void wv_save_position_callback(GObject *object, GAsyncResult *result,
                                      gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN("Unknown document type.");
    return;
  }
  int idx = document_get_notebook_page(doc);

  WebKitJavascriptResult *js_result;
  JSCValue *value;
  GError *error = NULL;

  js_result = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(object),
                                                    result, &error);
  if (!js_result) {
    g_warning("Error running javascript: %s", error->message);
    GERROR_FREE(error);
    return;
  }

  value = webkit_javascript_result_get_js_value(js_result);
  guint temp = jsc_value_to_int32(value);
  if (g_scrollY->len < idx) {
    g_array_insert_val(g_scrollY, idx, temp);
  } else {
    if (temp > 0) {
      g_array_insert_val(g_scrollY, idx, temp);
    }
  }

  webkit_javascript_result_unref(js_result);
}

static void wv_save_position() {
  char *script = g_strdup("window.scrollY");

  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, NULL,
                                 wv_save_position_callback, NULL);
  GFREE(script);
}

static void wv_apply_settings() {
  char *css_fn = NULL;
  webkit_user_content_manager_remove_all_style_sheets(g_wv_content_manager);

  webkit_settings_set_default_font_family(g_wv_settings,
                                          settings.default_font_family);

  // attach headers css
  css_fn = find_copy_css("preview-headers.css", PREVIEW_CSS_HEADERS);
  if (css_fn) {
    char *contents = NULL;
    size_t length = 0;
    if (g_file_get_contents(css_fn, &contents, &length, NULL)) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, NULL, NULL);
      webkit_user_content_manager_add_style_sheet(g_wv_content_manager,
                                                  stylesheet);
      webkit_user_style_sheet_unref(stylesheet);
      GFREE(contents);
    }
    GFREE(css_fn);
  }

  // attach extra_css
  css_fn = find_css(settings.extra_css);
  if (!css_fn) {
    if (g_strcmp0("dark.css", settings.extra_css) == 0) {
      css_fn = find_copy_css(settings.extra_css, PREVIEW_CSS_DARK);
    } else if (g_strcmp0("invert.css", settings.extra_css) == 0) {
      css_fn = find_copy_css(settings.extra_css, PREVIEW_CSS_INVERT);
    }
  }
  if (css_fn) {
    char *contents = NULL;
    size_t length = 0;
    if (g_file_get_contents(css_fn, &contents, &length, NULL)) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, NULL, NULL);
      webkit_user_content_manager_add_style_sheet(g_wv_content_manager,
                                                  stylesheet);
      webkit_user_style_sheet_unref(stylesheet);
      GFREE(contents);
    }
    GFREE(css_fn);
  }
}

static void wv_load_position() {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN("Unknown document type.");
    return;
  }
  int idx = document_get_notebook_page(doc);

  char *script;
  if (g_snippet) {
    script = g_strdup(
        "window.scrollTo(0, 0.2*document.documentElement.scrollHeight);");
  } else if (g_scrollY->len >= idx) {
    guint *temp = &g_array_index(g_scrollY, guint, idx);
    script = g_strdup_printf("window.scrollTo(0, %d);", *temp);
  } else {
    return;
  }
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, NULL, NULL,
                                 NULL);
  GFREE(script);
}

static void wv_loading_callback(WebKitWebView *web_view,
                                WebKitLoadEvent load_event,
                                gpointer user_data) {
  // don't wait for WEBKIT_LOAD_FINISHED, othewise preview will flicker
  wv_load_position();
}

static char *update_preview(const gboolean get_contents) {
  char *contents = NULL;

  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN("Unknown document type.");
    return NULL;
  }

  if (doc != g_current_doc) {
    g_current_doc = doc;
    set_filetype();
    set_snippets();
  }

  // save scroll position and set callback if needed
  wv_save_position();
  if (g_load_handle == 0) {
    g_load_handle =
        g_signal_connect_swapped(WEBKIT_WEB_VIEW(g_viewer), "load-changed",
                                 G_CALLBACK(wv_loading_callback), NULL);
  }

  char *uri = g_filename_to_uri(DOC_FILENAME(doc), NULL, NULL);
  if (!REGEX_CHK("file://", uri)) {
    GFREE(uri);
    uri = g_strdup("file:///tmp/blank.html");
  }

  char *basename = g_path_get_basename(DOC_FILENAME(doc));
  char *work_dir = g_path_get_dirname(DOC_FILENAME(doc));
  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  char *html = NULL;
  char *plain = NULL;
  GString *output = NULL;

  // extract snippet for large document
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

  // setup reg expressions to split head and body
  static GRegex *re_has_header = NULL;
  static GRegex *re_is_header = NULL;
  static GRegex *re_format = NULL;
  if (!re_has_header) {
    re_has_header = g_regex_new("^[^\\s:]+:\\s.*$", G_REGEX_MULTILINE, 0, NULL);
    re_is_header = g_regex_new("^(([^\\s:]+:.*)|([\\ \\t].*))$", 0, 0, NULL);
    re_format = g_regex_new("(?i)^(content-type|format):\\s*([^\\n]*)$",
                            G_REGEX_MULTILINE, 0, NULL);
  }

  // get format and split head/body
  GString *head = g_string_new(NULL);
  GString *body = g_string_new(NULL);

  char *format = NULL;
  if (!g_regex_match(re_has_header, text, 0, NULL)) {
    GSTRING_FREE(body);
    body = g_string_new(text);
  } else {
    GMatchInfo *match_info = NULL;

    // get format header
    if (g_regex_match(re_format, text, 0, &match_info)) {
      format = g_match_info_fetch(match_info, 2);
    }
    g_match_info_free(match_info);
    match_info = NULL;
    if (format) {
      g_filetype = get_filetype(format);
      GFREE(format);
    }
    // split head and body)
    gchar **texts = g_strsplit(text, "\n", -1);
    gboolean state_head = TRUE;
    int i = 0;
    while (texts[i] != NULL) {
      if (state_head) {
        if (g_regex_match(re_is_header, texts[i], 0, NULL)) {
          g_string_append(head, texts[i]);
          g_string_append(head, "\n");
          i++;
        } else {
          state_head = FALSE;
        }
      } else {
        g_string_append(body, texts[i]);
        g_string_append(body, "\n");
        i++;
      }
    }
    g_strfreev(texts);
    texts = NULL;
  }

  switch (g_filetype) {
    case HTML:
      if (REGEX_CHK("disable", settings.html_processor)) {
        plain = g_strdup("Preview of HTML documents has been disabled.");
      } else if (REGEX_CHK("pandoc", settings.html_processor)) {
        output = pandoc(work_dir, body->str, "html");
      } else {
        output = g_string_new(body->str);
      }
      break;
    case MARKDOWN:
      if (REGEX_CHK("disable", settings.markdown_processor)) {
        plain = g_strdup("Preview of Markdown documents has been disabled.");
      } else if (REGEX_CHK("pandoc", settings.markdown_processor)) {
        output = pandoc(work_dir, body->str, settings.pandoc_markdown);
      } else {
        html = cmark_markdown_to_html(body->str, body->len, 0);
        char *css_fn = find_copy_css("markdown.css", PREVIEW_CSS_MARKDOWN);
        if (css_fn) {
          plain = g_strjoin(NULL,
                            "<html><head><link rel='stylesheet' "
                            "type='text/css' href='file://",
                            css_fn, "'></head><body>", html, "</body></html>",
                            NULL);
          output = g_string_new(plain);
        } else {
          output = g_string_new(html);
        }

        GFREE(css_fn);
        GFREE(html);
        GFREE(plain);
      }
      break;
    case ASCIIDOC:
      if (REGEX_CHK("disable", settings.asciidoc_processor)) {
        plain = g_strdup("Preview of AsciiDoc documents has been disabled.");
      } else {
        output = asciidoctor(work_dir, body->str);
      }
      break;
    case DOCBOOK:
      output = pandoc(work_dir, body->str, "docbook");
      break;
    case LATEX:
      output = pandoc(work_dir, body->str, "latex");
      break;
    case REST:
      output = pandoc(work_dir, body->str, "rst");
      break;
    case TXT2TAGS:
      output = pandoc(work_dir, body->str, "t2t");
      break;
    case GFM:
      output = pandoc(work_dir, body->str, "gfm");
      break;
    case FOUNTAIN:
      if (REGEX_CHK("disable", settings.fountain_processor)) {
        plain = g_strdup("Preview of Fountain screenplays has been disabled.");
      } else {
        output = screenplain(work_dir, body->str, "html");
      }
      break;
    case TEXTILE:
      output = pandoc(work_dir, body->str, "textile");
      break;
    case DOKUWIKI:
      output = pandoc(work_dir, body->str, "dokuwiki");
      break;
    case TIKIWIKI:
      output = pandoc(work_dir, body->str, "tikiwiki");
      break;
    case VIMWIKI:
      output = pandoc(work_dir, body->str, "vimwiki");
      break;
    case TWIKI:
      output = pandoc(work_dir, body->str, "twiki");
      break;
    case MEDIAWIKI:
      output = pandoc(work_dir, body->str, "mediawiki");
      break;
    case WIKI:
      output = pandoc(work_dir, body->str, settings.wiki_default);
      break;
    case MUSE:
      output = pandoc(work_dir, body->str, "muse");
      break;
    case ORG:
      output = pandoc(work_dir, body->str, "org");
      break;
    case PLAIN:
    case EMAIL: {
      g_string_prepend(body, "<pre>");
      g_string_append(body, "</pre>");
      output = g_string_new(body->str);
      // plain = g_strdup(text);
    } break;
    case NONE:
    default:
      plain = g_strdup_printf("Unable to process type: %s, %s.",
                              doc->file_type->name, doc->encoding);
      break;
  }

  if (g_snippet) {
    GFREE(text);
  }

  if (plain) {
    WEBVIEW_WARN(plain);
    if (!get_contents) {
      GFREE(plain);
    } else {
      contents = plain;
    }
  } else if (output) {
    // combine head and body
    if (g_strcmp0(head->str, "") != 0 && g_strcmp0(head->str, "\n") != 0) {
      static GRegex *re_body = NULL;
      if (!re_body) {
        re_body = g_regex_new("(?i)(<body[^>]*>)", G_REGEX_MULTILINE, 0, NULL);
      }

      if (g_regex_match(re_body, output->str, 0, NULL)) {
        g_string_append(head, "</pre>\n");
        g_string_prepend(head, "\\1\n<pre class='geany_preview_headers'>");
        html = g_regex_replace(re_body, output->str, -1, 0, head->str, 0, NULL);
        GSTRING_FREE(output);
        output = g_string_new(html);
        GFREE(html);
      } else {
        g_string_append(head, "</pre>\n");
        g_string_prepend(head, "<pre class='geany_preview_headers'>");
        g_string_prepend(output, head->str);
      }
    }

    // output to webview
    webkit_web_context_clear_cache(g_wv_context);
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(g_viewer), output->str, uri);

    if (!get_contents) {
      GSTRING_FREE(output);
    } else
      contents = g_string_free(output, FALSE);
  }

  GSTRING_FREE(head);
  GSTRING_FREE(body);

  GFREE(uri);
  GFREE(basename);
  GFREE(work_dir);

  // restore scroll position
  wv_load_position();
  return contents;
}

static gboolean update_timeout_callback(gpointer user_data) {
  update_preview(FALSE);
  g_timeout_handle = 0;
  return FALSE;
}

static inline enum PreviewFileType get_filetype(char *format) {
  if (REGEX_CHK("gfm", format)) {
    return GFM;
  } else if (REGEX_CHK("fountain", format) || REGEX_CHK("spmd", format)) {
    return FOUNTAIN;
  } else if (REGEX_CHK("textile", format)) {
    return TEXTILE;
  } else if (REGEX_CHK("txt", format) || REGEX_CHK("plain", format)) {
    return PLAIN;
  } else if (REGEX_CHK("eml", format)) {
    return EMAIL;
  } else if (REGEX_CHK("wiki", format)) {
    if (REGEX_CHK("disable", settings.wiki_default)) {
      return NONE;
    } else if (REGEX_CHK("dokuwiki", format)) {
      return DOKUWIKI;
    } else if (REGEX_CHK("tikiwiki", format)) {
      return TIKIWIKI;
    } else if (REGEX_CHK("vimwiki", format)) {
      return VIMWIKI;
    } else if (REGEX_CHK("twiki", format)) {
      return TWIKI;
    } else if (REGEX_CHK("mediawiki", format) ||
               REGEX_CHK("wikipedia", format)) {
      return MEDIAWIKI;
    } else {
      return WIKI;
    }
  } else if (REGEX_CHK("muse", format)) {
    return MUSE;
  } else if (REGEX_CHK("org", format)) {
    return ORG;
  } else if (REGEX_CHK("html", format)) {
    return HTML;
  } else if (REGEX_CHK("markdown", format)) {
    return MARKDOWN;
  } else if (REGEX_CHK("asciidoc", format)) {
    return ASCIIDOC;
  } else if (REGEX_CHK("docbook", format)) {
    return DOCBOOK;
  } else if (REGEX_CHK("latex", format)) {
    return LATEX;
  } else if (REGEX_CHK("rest", format) ||
             REGEX_CHK("restructuredtext", format)) {
    return REST;
  } else if (REGEX_CHK("txt2tags", format) || REGEX_CHK("t2t", format)) {
    return TXT2TAGS;
  }
  return NONE;
}

static inline void set_filetype() {
  GeanyDocument *doc = document_get_current();

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_HTML:
      g_filetype = HTML;
      break;
    case GEANY_FILETYPES_MARKDOWN:
      g_filetype = MARKDOWN;
      break;
    case GEANY_FILETYPES_ASCIIDOC:
      g_filetype = ASCIIDOC;
      break;
    case GEANY_FILETYPES_DOCBOOK:
      g_filetype = DOCBOOK;
      break;
    case GEANY_FILETYPES_LATEX:
      g_filetype = LATEX;
      break;
    case GEANY_FILETYPES_REST:
      g_filetype = REST;
      break;
    case GEANY_FILETYPES_TXT2TAGS:
      g_filetype = TXT2TAGS;
      break;
    case GEANY_FILETYPES_NONE:
      if (!settings.extended_types) {
        g_filetype = NONE;
        break;
      } else {
        char *basename = g_path_get_basename(DOC_FILENAME(doc));
        g_filetype = get_filetype(basename);
        GFREE(basename);
      }
      break;
    default:
      g_filetype = NONE;
      break;
  }
}

static inline void set_snippets() {
  GeanyDocument *doc = document_get_current();

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
      GFREE(basename);
    }  // no break; need to check pandoc setting
    case GEANY_FILETYPES_LATEX:
    case GEANY_FILETYPES_DOCBOOK:
    case GEANY_FILETYPES_REST:
    case GEANY_FILETYPES_TXT2TAGS:
    default:
      if (!settings.snippet_pandoc) {
        g_snippet = FALSE;
      }
      break;
  }
}

static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN("Unknown document type.");
    return FALSE;
  }

  int length = sci_get_length(doc->editor->sci);

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
  if (gtk_notebook_get_current_page(nb) != g_nb_page_num ||
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
  if (settings.column_marker_enable && DOC_VALID(doc)) {
    scintilla_send_message(doc->editor->sci, SCI_SETEDGEMODE, 3, 3);
    scintilla_send_message(doc->editor->sci, SCI_MULTIEDGECLEARALL, 0, 0);

    if (settings.column_marker_columns != NULL &&
        settings.column_marker_colors != NULL) {
      for (int i = 0; i < settings.column_marker_count; i++) {
        scintilla_send_message(doc->editor->sci, SCI_MULTIEDGEADDLINE,
                               settings.column_marker_columns[i],
                               settings.column_marker_colors[i]);
      }
    }
  }

  g_current_doc = NULL;
  update_preview(FALSE);
}
