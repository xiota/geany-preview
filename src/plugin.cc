/*
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmark-gfm.h>

#include "formats.h"
#include "fountain.h"
#include "plugin.h"
#include "prefs.h"
#include "process.h"

#define WEBVIEW_WARN(msg) \
  webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(g_viewer), (msg))

/* ********************
 * Globals
 */
GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GtkWidget *g_preview_menu;
static GtkWidget *g_viewer = nullptr;
static WebKitSettings *g_wv_settings = nullptr;
static WebKitWebContext *g_wv_context = nullptr;
WebKitUserContentManager *g_wv_content_manager = nullptr;
static GtkWidget *g_scrolled_win = nullptr;
static guint g_nb_page_num = 0;
static GArray *g_scrollY = nullptr;
static gulong g_load_handle = 0;
static guint g_timeout_handle = 0;
static gboolean g_snippet = false;
static gulong g_handle_sidebar_switch_page = 0;
static gulong g_handle_sidebar_show = 0;

static GeanyDocument *g_current_doc = nullptr;

extern struct PreviewSettings settings;

PreviewFileType g_filetype = PREVIEW_FILETYPE_NONE;
GtkNotebook *g_sidebar_notebook = nullptr;
GtkWidget *g_sidebar_preview_page = nullptr;

/* ********************
 * Plugin Setup
 */
PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Preview",
                _("Preview pane for HTML, Markdown, and other lightweight "
                  "markup formats."),
                "0.01.1", "xiota")

void plugin_init(G_GNUC_UNUSED GeanyData *data) {
  GEANY_PSC("geany-startup-complete", on_document_signal);
  GEANY_PSC("editor-notify", on_editor_notify);
  GEANY_PSC("document-activate", on_document_signal);
  GEANY_PSC("document-filetype-set", on_document_signal);
  GEANY_PSC("document-new", on_document_signal);
  GEANY_PSC("document-open", on_document_signal);
  GEANY_PSC("document-reload", on_document_signal);

  // Set keyboard shortcuts
  GeanyKeyGroup *group = plugin_set_key_group(
      geany_plugin, "Preview", 1, (GeanyKeyGroupCallback)on_key_binding);

  keybindings_set_item(group, PREVIEW_KEY_TOGGLE_EDITOR, nullptr, 0,
                       GdkModifierType(0), "preview_toggle_editor",
                       _("Toggle focus between the editor and preview pane."),
                       nullptr);

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
  g_scrollY = g_array_new(true, true, sizeof(gint32));

  open_settings();

  // set up menu
  GeanyKeyGroup *group;
  GtkWidget *item;

  g_preview_menu = gtk_menu_item_new_with_label(_("Preview"));
  ui_add_document_sensitive(g_preview_menu);

  GtkWidget *submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(g_preview_menu), submenu);

  item = gtk_menu_item_new_with_label(_("Export to HTML..."));
  g_signal_connect(item, "activate", G_CALLBACK(on_menu_export_html), nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Edit Config File"));
  g_signal_connect(item, "activate", G_CALLBACK(on_pref_edit_config), nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Reload Config File"));
  g_signal_connect(item, "activate", G_CALLBACK(on_pref_reload_config),
                   nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Open Config Folder"));
  g_signal_connect(item, "activate", G_CALLBACK(on_pref_open_config_folder),
                   nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Preferences"));
  g_signal_connect(item, "activate", G_CALLBACK(on_menu_preferences), nullptr);
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

  g_scrolled_win = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_container_add(GTK_CONTAINER(g_scrolled_win), g_viewer);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_scrolled_win),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  g_sidebar_notebook = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  g_nb_page_num = gtk_notebook_append_page(g_sidebar_notebook, g_scrolled_win,
                                           gtk_label_new(_("Preview")));

  gtk_widget_show_all(g_scrolled_win);
  gtk_notebook_set_current_page(g_sidebar_notebook, g_nb_page_num);

  g_sidebar_preview_page =
      gtk_notebook_get_nth_page(g_sidebar_notebook, g_nb_page_num);

  // signal handlers to update notebook
  g_handle_sidebar_switch_page =
      g_signal_connect(GTK_WIDGET(g_sidebar_notebook), "switch-page",
                       G_CALLBACK(on_sidebar_switch_page), nullptr);

  g_handle_sidebar_show =
      g_signal_connect(GTK_WIDGET(g_sidebar_notebook), "show",
                       G_CALLBACK(on_sidebar_show), nullptr);

  wv_apply_settings();

  WEBVIEW_WARN(_("Loading."));

  // preview may need to be updated after a delay on first use
  if (g_timeout_handle == 0) {
    g_timeout_handle = g_timeout_add(2000, update_timeout_callback, nullptr);
  }
  return true;
}

static void preview_cleanup(GeanyPlugin *plugin, gpointer data) {
  // fmt_prefs_deinit();
  gtk_notebook_remove_page(g_sidebar_notebook, g_nb_page_num);

  gtk_widget_destroy(g_viewer);
  gtk_widget_destroy(g_scrolled_win);

  gtk_widget_destroy(g_preview_menu);

  g_clear_signal_handler(&g_handle_sidebar_switch_page,
                         GTK_WIDGET(g_sidebar_notebook));
  g_clear_signal_handler(&g_handle_sidebar_show,
                         GTK_WIDGET(g_sidebar_notebook));
}

static GtkWidget *preview_configure(GeanyPlugin *plugin, GtkDialog *dialog,
                                    gpointer pdata) {
  GtkWidget *box, *btn;
  char *tooltip;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

  tooltip = g_strdup(_("Save the active settings to the config file."));
  btn = gtk_button_new_with_label(_("Save Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_save_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip = g_strdup(
      _("Reload settings from the config file.  May be used "
        "to apply preferences after editing without restarting Geany."));
  btn = gtk_button_new_with_label(_("Reload Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_reload_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip =
      g_strdup(_("Delete the current config file and restore the default "
                 "file with explanatory comments."));
  btn = gtk_button_new_with_label(_("Reset Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_reset_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip = g_strdup(_("Open the config file in Geany for editing."));
  btn = gtk_button_new_with_label(_("Edit Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_edit_config), dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  tooltip =
      g_strdup(_("Open the config folder in the default file manager.  The "
                 "config folder "
                 "contains the stylesheets, which may be edited."));
  btn = gtk_button_new_with_label(_("Open Config Folder"));
  g_signal_connect(btn, "clicked", G_CALLBACK(on_pref_open_config_folder),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  GFREE(tooltip);

  return box;
}

/* ********************
 * Keybinding and Preferences Callbacks
 */
static void on_toggle_editor_preview() {
  GeanyDocument *doc = document_get_current();
  if (doc != nullptr) {
    GtkWidget *sci = GTK_WIDGET(doc->editor->sci);
    if (gtk_widget_has_focus(sci) &&
        gtk_widget_is_visible(GTK_WIDGET(g_sidebar_notebook))) {
      gtk_notebook_set_current_page(g_sidebar_notebook, g_nb_page_num);
      GtkWidget *page =
          gtk_notebook_get_nth_page(g_sidebar_notebook, g_nb_page_num);
      gtk_widget_child_focus(page, GTK_DIR_TAB_FORWARD);
    } else {
      keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    }
  }
}

static bool on_key_binding(int key_id) {
  switch (key_id) {
    case PREVIEW_KEY_TOGGLE_EDITOR:
      on_toggle_editor_preview();
      break;
    default:
      return false;
  }
  return true;
}

static void on_pref_open_config_folder(GtkWidget *self, GtkWidget *dialog) {
  char *conf_dn = g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", nullptr);

  char *command;
  command = g_strdup_printf("xdg-open \"%s\"", conf_dn);
  if (system(command)) {
    // ignore return value
  }
  GFREE(conf_dn);
  GFREE(command);
}

static void on_pref_edit_config(GtkWidget *self, GtkWidget *dialog) {
  open_settings();
  char *conf_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", "preview.conf", nullptr);
  GeanyDocument *doc = document_open_file(conf_fn, false, nullptr, nullptr);
  document_reload_force(doc, nullptr);
  GFREE(conf_fn);

  if (dialog != nullptr) {
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

// from markdown plugin
static char *replace_extension(char const *utf8_fn, char const *new_ext) {
  char *fn_noext, *new_fn, *dot;
  fn_noext = g_filename_from_utf8(utf8_fn, -1, nullptr, nullptr, nullptr);
  dot = strrchr(fn_noext, '.');
  if (dot != nullptr) {
    *dot = '\0';
  }
  new_fn = g_strconcat(fn_noext, new_ext, nullptr);
  GFREE(fn_noext);
  return new_fn;
}

// from markdown plugin
static void on_menu_export_html(GtkWidget *self, GtkWidget *dialog) {
  GtkFileFilter *filter;
  char *fn;
  gboolean saved = false;

  GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));

  GtkWidget *save_dialog = gtk_file_chooser_dialog_new(
      _("Save As HTML"), GTK_WINDOW(geany_data->main_widgets->window),
      GTK_FILE_CHOOSER_ACTION_SAVE, _("Cancel"), GTK_RESPONSE_CANCEL, _("Save"),
      GTK_RESPONSE_ACCEPT, nullptr);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(save_dialog),
                                                 true);

  fn = replace_extension(DOC_FILENAME(doc), ".html");
  if (g_file_test(fn, G_FILE_TEST_EXISTS)) {
    // If the file exists, GtkFileChooser will change to the correct
    // directory and show the base name as a suggestion.
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(save_dialog), fn);
  } else {
    // If the file doesn't exist, change the directory and give a
    // suggested name for the file, since GtkFileChooser won't do it.
    char *dn = g_path_get_dirname(fn);
    char *bn = g_path_get_basename(fn);
    char *utf8_name;
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(save_dialog), dn);
    GFREE(dn);
    utf8_name = g_filename_to_utf8(bn, -1, nullptr, nullptr, nullptr);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog), utf8_name);
    GFREE(bn);
    GFREE(utf8_name);
  }
  GFREE(fn);

  // add file type filters to the chooser
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, _("HTML Files"));
  gtk_file_filter_add_mime_type(filter, "text/html");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, _("All Files"));
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

  while (!saved &&
         gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT) {
    char *html = update_preview(true);
    GError *error = nullptr;
    fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));
    if (!g_file_set_contents(fn, html, -1, &error)) {
      dialogs_show_msgbox(GTK_MESSAGE_ERROR,
                          _("Failed to export HTML to file '%s': %s"), fn,
                          error->message);
      g_error_free(error);
    } else {
      saved = true;
    }
    GFREE(fn);
    GFREE(html);
  }

  gtk_widget_destroy(save_dialog);
}

/* ********************
 * Sidebar Functions and Callbacks
 */
static void on_sidebar_switch_page(GtkNotebook *nb, GtkWidget *page,
                                   guint page_num, gpointer user_data) {
  if (g_nb_page_num == page_num) {
    g_current_doc = nullptr;
    update_preview(false);
  }
}

static void on_sidebar_show(GtkNotebook *nb, gpointer user_data) {
  if (gtk_notebook_get_current_page(nb) == g_nb_page_num) {
    g_current_doc = nullptr;
    update_preview(false);
  }
}

// from geany keybindings.c
static GtkWidget *find_focus_widget(GtkWidget *widget) {
  GtkWidget *focus = nullptr;

  // optimized simple case
  if (GTK_IS_BIN(widget)) {
    focus = find_focus_widget(gtk_bin_get_child(GTK_BIN(widget)));
  } else if (GTK_IS_CONTAINER(widget)) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
    GList *node;

    for (node = children; node && !focus; node = node->next)
      focus = find_focus_widget(GTK_WIDGET(node->data));
    g_list_free(children);
  }

  // Some containers handled above might not have children and be what
  // we want to focus (e.g. GtkTreeView), so focus that if possible and
  // we don't have anything better
  if (!focus && gtk_widget_get_can_focus(widget)) {
    focus = widget;
  }
  return focus;
}

/* ********************
 * WebView Scrollbar Position
 */

static void wv_save_position_callback(GObject *object, GAsyncResult *result,
                                      gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return;
  }
  int idx = document_get_notebook_page(doc);

  WebKitJavascriptResult *js_result;
  JSCValue *value;
  GError *error = nullptr;

  js_result = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(object),
                                                    result, &error);
  if (!js_result) {
    g_warning(_("Error running javascript: %s"), error->message);
    GERROR_FREE(error);
    return;
  }

  value = webkit_javascript_result_get_js_value(js_result);
  guint temp = jsc_value_to_int32(value);
  if (g_scrollY->len < idx) {
    g_array_insert_val(g_scrollY, idx, temp);
  } else {
    if (temp > 0) {
      gint32 *scrollY = &g_array_index(g_scrollY, gint32, idx);
      *scrollY = temp;
    }
  }

  webkit_javascript_result_unref(js_result);
}

static void wv_save_position() {
  char *script = g_strdup("window.scrollY");

  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, nullptr,
                                 wv_save_position_callback, nullptr);
  GFREE(script);
}

static void wv_apply_settings() {
  char *css_fn = nullptr;
  webkit_user_content_manager_remove_all_style_sheets(g_wv_content_manager);

  webkit_settings_set_default_font_family(g_wv_settings,
                                          settings.default_font_family);

  // attach headers css
  css_fn = find_copy_css("preview-headers.css", PREVIEW_CSS_HEADERS);
  if (css_fn) {
    char *contents = nullptr;
    size_t length = 0;
    if (g_file_get_contents(css_fn, &contents, &length, nullptr)) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, nullptr, nullptr);
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
    if (strcmp("dark.css", settings.extra_css) == 0) {
      css_fn = find_copy_css(settings.extra_css, PREVIEW_CSS_DARK);
    } else if (strcmp("invert.css", settings.extra_css) == 0) {
      css_fn = find_copy_css(settings.extra_css, PREVIEW_CSS_INVERT);
    }
  }
  if (css_fn) {
    char *contents = nullptr;
    size_t length = 0;
    if (g_file_get_contents(css_fn, &contents, &length, nullptr)) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, nullptr, nullptr);
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
    WEBVIEW_WARN(_("Unknown document type."));
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
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(g_viewer), script, nullptr,
                                 nullptr, nullptr);
  GFREE(script);
}

static void wv_loading_callback(WebKitWebView *web_view,
                                WebKitLoadEvent load_event,
                                gpointer user_data) {
  // don't wait for WEBKIT_LOAD_FINISHED, othewise preview will flicker
  wv_load_position();
}

/* ********************
 * Other Functions
 */

static gboolean update_timeout_callback(gpointer user_data) {
  update_preview(false);
  g_timeout_handle = 0;
  return false;
}

static char *update_preview(gboolean const get_contents) {
  char *contents = nullptr;

  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return nullptr;
  }

  if (doc != g_current_doc) {
    g_current_doc = doc;
    set_filetype();
    set_snippets();
  }

  // don't use snippets when exporting html
  if (get_contents) {
    g_snippet = false;
  }

  // save scroll position and set callback if needed
  wv_save_position();
  if (g_load_handle == 0) {
    g_load_handle =
        g_signal_connect_swapped(WEBKIT_WEB_VIEW(g_viewer), "load-changed",
                                 G_CALLBACK(wv_loading_callback), nullptr);
  }
  char *uri = g_filename_to_uri(DOC_FILENAME(doc), nullptr, nullptr);
  if (!uri || strlen(uri) < 7 || strncmp(uri, "file://", 7) != 0) {
    GFREE(uri);
    uri = g_strdup("file:///tmp/blank.html");
  }

  char *work_dir = g_path_get_dirname(DOC_FILENAME(doc));
  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  char *html = nullptr;
  char *plain = nullptr;
  GString *output = nullptr;

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
  static GRegex *re_has_header = nullptr;
  static GRegex *re_is_header = nullptr;
  static GRegex *re_format = nullptr;
  if (!re_has_header) {
    re_has_header = g_regex_new("^[^\\s:]+:\\s.*$", G_REGEX_MULTILINE,
                                GRegexMatchFlags(0), nullptr);
    re_is_header =
        g_regex_new("^(([^\\s:]+:.*)|([\\ \\t].*))$", GRegexCompileFlags(0),
                    GRegexMatchFlags(0), nullptr);
    re_format = g_regex_new("(?i)^(content-type|format):\\s*([^\\n]*)$",
                            G_REGEX_MULTILINE, GRegexMatchFlags(0), nullptr);
  }

  // get format and split head/body
  GString *head = g_string_new(nullptr);
  GString *body = g_string_new(nullptr);

  char *format = nullptr;
  if (text &&
      !g_regex_match(re_has_header, text, GRegexMatchFlags(0), nullptr)) {
    GSTRING_FREE(body);
    body = g_string_new(text);
  } else if (text) {
    GMatchInfo *match_info = nullptr;

    // get format header
    if (g_regex_match(re_format, text, GRegexMatchFlags(0), &match_info)) {
      format = g_match_info_fetch(match_info, 2);
    }
    g_match_info_free(match_info);
    match_info = nullptr;
    if (format) {
      g_filetype = get_filetype(format);
      GFREE(format);
    }

    // split head and body
    if (g_filetype == PREVIEW_FILETYPE_FOUNTAIN) {
      // fountain handles its own headers
      body = g_string_new(text);
    } else {
      char **texts = g_strsplit(text, "\n", -1);
      gboolean state_head = true;
      int i = 0;
      while (texts[i] != nullptr) {
        if (state_head) {
          if (*texts[i] != '\0') {
            g_string_append(head, texts[i]);
            g_string_append(head, "\n");
            i++;
          } else {
            state_head = false;
          }
        } else {
          g_string_append(body, texts[i]);
          g_string_append(body, "\n");
          i++;
        }
      }
      g_strfreev(texts);
      texts = nullptr;
    }
  }

  switch (g_filetype) {
    case PREVIEW_FILETYPE_HTML:
      if (strcmp("disable", settings.html_processor) == 0) {
        plain = g_strdup(_("Preview of HTML documents has been disabled."));
      } else if (SUBSTR("pandoc", settings.html_processor)) {
        output = pandoc(work_dir, body->str, "html");
      } else {
        output = g_string_new(body->str);
      }
      break;
    case PREVIEW_FILETYPE_MARKDOWN:
      if (strcmp("disable", settings.markdown_processor) == 0) {
        plain = g_strdup(_("Preview of Markdown documents has been disabled."));
      } else if (SUBSTR("pandoc", settings.markdown_processor)) {
        output = pandoc(work_dir, body->str, settings.pandoc_markdown);
      } else {
        html = cmark_markdown_to_html(body->str, body->len, 0);
        char *css_fn = find_copy_css("markdown.css", PREVIEW_CSS_MARKDOWN);
        if (css_fn) {
          plain = g_strjoin(nullptr,
                            "<html><head><link rel='stylesheet' "
                            "type='text/css' href='file://",
                            css_fn, "'></head><body>", html, "</body></html>",
                            nullptr);
          output = g_string_new(plain);
        } else {
          output = g_string_new(html);
        }

        GFREE(css_fn);
        GFREE(html);
        GFREE(plain);
      }
      break;
    case PREVIEW_FILETYPE_ASCIIDOC:
      if (strcmp("disable", settings.asciidoc_processor) == 0) {
        plain = g_strdup(_("Preview of AsciiDoc documents has been disabled."));
      } else {
        output = asciidoctor(work_dir, body->str);
      }
      break;
    case PREVIEW_FILETYPE_DOCBOOK:
      output = pandoc(work_dir, body->str, "docbook");
      break;
    case PREVIEW_FILETYPE_LATEX:
      output = pandoc(work_dir, body->str, "latex");
      break;
    case PREVIEW_FILETYPE_REST:
      output = pandoc(work_dir, body->str, "rst");
      break;
    case PREVIEW_FILETYPE_TXT2TAGS:
      output = pandoc(work_dir, body->str, "t2t");
      break;
    case PREVIEW_FILETYPE_GFM:
      output = pandoc(work_dir, body->str, "gfm");
      break;
    case PREVIEW_FILETYPE_FOUNTAIN:
      if (strcmp("disable", settings.fountain_processor) == 0) {
        plain =
            g_strdup(_("Preview of Fountain screenplays has been disabled."));
      } else if (strcmp("screenplain", settings.fountain_processor) == 0) {
        output = screenplain(work_dir, body->str, "html");
      } else {
        char *css_fn = find_copy_css("fountain.css", PREVIEW_CSS_FOUNTAIN);
        std::string out = ftn2html(body->str, css_fn);
        output = g_string_new(out.c_str());

        GFREE(css_fn);
      }
      break;
    case PREVIEW_FILETYPE_TEXTILE:
      output = pandoc(work_dir, body->str, "textile");
      break;
    case PREVIEW_FILETYPE_DOKUWIKI:
      output = pandoc(work_dir, body->str, "dokuwiki");
      break;
    case PREVIEW_FILETYPE_TIKIWIKI:
      output = pandoc(work_dir, body->str, "tikiwiki");
      break;
    case PREVIEW_FILETYPE_VIMWIKI:
      output = pandoc(work_dir, body->str, "vimwiki");
      break;
    case PREVIEW_FILETYPE_TWIKI:
      output = pandoc(work_dir, body->str, "twiki");
      break;
    case PREVIEW_FILETYPE_MEDIAWIKI:
      output = pandoc(work_dir, body->str, "mediawiki");
      break;
    case PREVIEW_FILETYPE_WIKI:
      output = pandoc(work_dir, body->str, settings.wiki_default);
      break;
    case PREVIEW_FILETYPE_MUSE:
      output = pandoc(work_dir, body->str, "muse");
      break;
    case PREVIEW_FILETYPE_ORG:
      output = pandoc(work_dir, body->str, "org");
      break;
    case PREVIEW_FILETYPE_PLAIN:
    case PREVIEW_FILETYPE_EMAIL: {
      g_string_prepend(body, "<pre>");
      g_string_append(body, "</pre>");
      output = g_string_new(body->str);
      // plain = g_strdup(text);
    } break;
    case PREVIEW_FILETYPE_NONE:
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
    if (strcmp(head->str, "") != 0 && strcmp(head->str, "\n") != 0) {
      static GRegex *re_body = nullptr;
      if (!re_body) {
        re_body = g_regex_new("(?i)(<body[^>]*>)", G_REGEX_MULTILINE,
                              GRegexMatchFlags(0), nullptr);
      }

      if (output->str &&
          g_regex_match(re_body, output->str, GRegexMatchFlags(0), nullptr)) {
        g_string_append(head, "</pre>\n");
        g_string_prepend(head, "\\1\n<pre class='geany_preview_headers'>");
        html = g_regex_replace(re_body, output->str, -1, 0, head->str,
                               GRegexMatchFlags(0), nullptr);
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
      contents = g_string_free(output, false);
  }

  GSTRING_FREE(head);
  GSTRING_FREE(body);

  GFREE(uri);
  GFREE(work_dir);

  // restore scroll position
  wv_load_position();
  return contents;
}

static PreviewFileType get_filetype(char const *fn) {
  if (!fn) {
    return PREVIEW_FILETYPE_NONE;
  }

  char *bn = g_path_get_basename(fn);
  char *format = g_utf8_strdown(bn, -1);
  if (!bn || !format) {
    GFREE(bn);
    GFREE(format);
    return PREVIEW_FILETYPE_NONE;
  }

  PreviewFileType filetype = PREVIEW_FILETYPE_NONE;

  if (SUBSTR("gfm", format)) {
    filetype = PREVIEW_FILETYPE_GFM;
  } else if (SUBSTR("fountain", format) || SUBSTR("spmd", format)) {
    filetype = PREVIEW_FILETYPE_FOUNTAIN;
  } else if (SUBSTR("textile", format)) {
    filetype = PREVIEW_FILETYPE_TEXTILE;
  } else if (SUBSTR("txt", format) || SUBSTR("plain", format)) {
    filetype = PREVIEW_FILETYPE_PLAIN;
  } else if (SUBSTR("eml", format)) {
    filetype = PREVIEW_FILETYPE_EMAIL;
  } else if (SUBSTR("wiki", format)) {
    if (strcmp("disable", settings.wiki_default) == 0) {
      filetype = PREVIEW_FILETYPE_NONE;
    } else if (SUBSTR("dokuwiki", format)) {
      filetype = PREVIEW_FILETYPE_DOKUWIKI;
    } else if (SUBSTR("tikiwiki", format)) {
      filetype = PREVIEW_FILETYPE_TIKIWIKI;
    } else if (SUBSTR("vimwiki", format)) {
      filetype = PREVIEW_FILETYPE_VIMWIKI;
    } else if (SUBSTR("twiki", format)) {
      filetype = PREVIEW_FILETYPE_TWIKI;
    } else if (SUBSTR("mediawiki", format) || SUBSTR("wikipedia", format)) {
      filetype = PREVIEW_FILETYPE_MEDIAWIKI;
    } else {
      filetype = PREVIEW_FILETYPE_WIKI;
    }
  } else if (SUBSTR("muse", format)) {
    filetype = PREVIEW_FILETYPE_MUSE;
  } else if (SUBSTR("org", format)) {
    filetype = PREVIEW_FILETYPE_ORG;
  } else if (SUBSTR("html", format)) {
    filetype = PREVIEW_FILETYPE_HTML;
  } else if (SUBSTR("markdown", format)) {
    filetype = PREVIEW_FILETYPE_MARKDOWN;
  } else if (SUBSTR("asciidoc", format)) {
    filetype = PREVIEW_FILETYPE_ASCIIDOC;
  } else if (SUBSTR("docbook", format)) {
    filetype = PREVIEW_FILETYPE_DOCBOOK;
  } else if (SUBSTR("latex", format)) {
    filetype = PREVIEW_FILETYPE_LATEX;
  } else if (SUBSTR("rest", format) || SUBSTR("restructuredtext", format)) {
    filetype = PREVIEW_FILETYPE_REST;
  } else if (SUBSTR("txt2tags", format) || SUBSTR("t2t", format)) {
    filetype = PREVIEW_FILETYPE_TXT2TAGS;
  }

  GFREE(bn);
  GFREE(format);
  return filetype;
}

static inline void set_filetype() {
  GeanyDocument *doc = document_get_current();

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_HTML:
      g_filetype = PREVIEW_FILETYPE_HTML;
      break;
    case GEANY_FILETYPES_MARKDOWN:
      g_filetype = PREVIEW_FILETYPE_MARKDOWN;
      break;
    case GEANY_FILETYPES_ASCIIDOC:
      g_filetype = PREVIEW_FILETYPE_ASCIIDOC;
      break;
    case GEANY_FILETYPES_DOCBOOK:
      g_filetype = PREVIEW_FILETYPE_DOCBOOK;
      break;
    case GEANY_FILETYPES_LATEX:
      g_filetype = PREVIEW_FILETYPE_LATEX;
      break;
    case GEANY_FILETYPES_REST:
      g_filetype = PREVIEW_FILETYPE_REST;
      break;
    case GEANY_FILETYPES_TXT2TAGS:
      g_filetype = PREVIEW_FILETYPE_TXT2TAGS;
      break;
    case GEANY_FILETYPES_NONE:
      if (!settings.extended_types) {
        g_filetype = PREVIEW_FILETYPE_NONE;
        break;
      } else {
        g_filetype = get_filetype(DOC_FILENAME(doc));
      }
      break;
    default:
      g_filetype = PREVIEW_FILETYPE_NONE;
      break;
  }
}

static inline void set_snippets() {
  GeanyDocument *doc = document_get_current();

  int length = sci_get_length(doc->editor->sci);
  if (length > settings.snippet_trigger) {
    g_snippet = true;
  } else {
    g_snippet = false;
  }

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_ASCIIDOC:
      if (!settings.snippet_asciidoctor) {
        g_snippet = false;
      }
      break;
    case GEANY_FILETYPES_HTML:
      if (!settings.snippet_html) {
        g_snippet = false;
      }
      break;
    case GEANY_FILETYPES_MARKDOWN:
      if (!settings.snippet_markdown) {
        g_snippet = false;
      }
      break;
    case GEANY_FILETYPES_NONE: {
      char *bn = g_path_get_basename(DOC_FILENAME(doc));
      char *format = g_utf8_strdown(bn, -1);
      if (format) {
        if (SUBSTR("fountain", format) || SUBSTR("spmd", format)) {
          if (!settings.snippet_screenplain) {
            g_snippet = false;
          }
        }
        GFREE(bn);
        GFREE(format);
      }
    }  // no break; need to check pandoc setting
    case GEANY_FILETYPES_LATEX:
    case GEANY_FILETYPES_DOCBOOK:
    case GEANY_FILETYPES_REST:
    case GEANY_FILETYPES_TXT2TAGS:
    default:
      if (!settings.snippet_pandoc) {
        g_snippet = false;
      }
      break;
  }
}

static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                 SCNotification *notif, gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return false;
  }

  int length = sci_get_length(doc->editor->sci);

  gboolean need_update = false;
  if (notif->nmhdr.code == SCN_UPDATEUI && g_snippet &&
      (notif->updated & (SC_UPDATE_CONTENT | SC_UPDATE_SELECTION))) {
    need_update = true;
  }

  if (notif->nmhdr.code == SCN_MODIFIED && notif->length > 0) {
    if (notif->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      need_update = true;
    }
  }

  if (gtk_notebook_get_current_page(g_sidebar_notebook) != g_nb_page_num ||
      !gtk_widget_is_visible(GTK_WIDGET(g_sidebar_notebook))) {
    // no updates when preview pane is hidden
    return false;
  }

  if (need_update && g_timeout_handle == 0) {
    if (doc->file_type->id != GEANY_FILETYPES_ASCIIDOC &&
        doc->file_type->id != GEANY_FILETYPES_NONE) {
      // delay for faster programs
      double _tt = (double)length * settings.size_factor_fast;
      int timeout = (int)_tt > settings.update_interval_fast
                        ? (int)_tt
                        : settings.update_interval_fast;
      g_timeout_handle =
          g_timeout_add(timeout, update_timeout_callback, nullptr);
    } else {
      // delay for slower programs
      double _tt = (double)length * settings.size_factor_slow;
      int timeout = (int)_tt > settings.update_interval_slow
                        ? (int)_tt
                        : settings.update_interval_slow;
      g_timeout_handle =
          g_timeout_add(timeout, update_timeout_callback, nullptr);
    }
  }

  return false;
}

static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data) {
  if (gtk_notebook_get_current_page(g_sidebar_notebook) != g_nb_page_num ||
      !gtk_widget_is_visible(GTK_WIDGET(g_sidebar_notebook))) {
    // no updates when preview pane is hidden
    return;
  }

  g_current_doc = nullptr;
  update_preview(false);
}
