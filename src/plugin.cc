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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmark-gfm.h>

#include "auxiliary.h"
#include "formats.h"
#include "fountain.h"
#include "plugin.h"
#include "prefs.h"
#include "process.h"

#define WEBVIEW_WARN(msg) \
  webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(gWebView), (msg))

/* ********************
 * Globals
 */
GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GtkWidget *gPreviewMenu;
static GtkWidget *gWebView = nullptr;
static WebKitSettings *gWebViewSettings = nullptr;
static WebKitWebContext *gWebViewContext = nullptr;
WebKitUserContentManager *gWebViewContentManager = nullptr;
static GtkWidget *gScrolledWindow = nullptr;
static uint gPreviewSideBarPageNumber = 0;
static GArray *g_scrollY = nullptr;
static ulong gHandleLoadFinished = 0;
static ulong gHandleTimeout = 0;
static bool gSnippetActive = false;
static ulong gHandleSidebarSwitchPage = 0;
static ulong gHandleSidebarShow = 0;

static GeanyDocument *gCurrentDocument = nullptr;

extern struct PreviewSettings settings;

PreviewFileType gFileType = PREVIEW_FILETYPE_NONE;
GtkNotebook *gSideBarNotebook = nullptr;
GtkWidget *gSideBarPreviewPage = nullptr;

/* ********************
 * Plugin Setup
 */
PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Preview",
                _("Preview pane for HTML, Markdown, and other lightweight "
                  "markup formats."),
                "0.01.1", "xiota")

void plugin_init(GeanyData *data) {
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

static bool preview_init(GeanyPlugin *plugin, gpointer data) {
  geany_plugin = plugin;
  geany_data = plugin->geany_data;
  g_scrollY = g_array_new(true, true, sizeof(int));

  open_settings();

  // set up menu
  GeanyKeyGroup *group;
  GtkWidget *item;

  gPreviewMenu = gtk_menu_item_new_with_label(_("Preview"));
  ui_add_document_sensitive(gPreviewMenu);

  GtkWidget *submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(gPreviewMenu), submenu);

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

  gtk_widget_show_all(gPreviewMenu);

  gtk_menu_shell_append(GTK_MENU_SHELL(geany_data->main_widgets->tools_menu),
                        gPreviewMenu);

  // set up webview
  gWebViewSettings = webkit_settings_new();
  gWebView = webkit_web_view_new_with_settings(gWebViewSettings);

  gWebViewContext = webkit_web_view_get_context(WEBKIT_WEB_VIEW(gWebView));
  gWebViewContentManager =
      webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(gWebView));

  gScrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_container_add(GTK_CONTAINER(gScrolledWindow), gWebView);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gScrolledWindow),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gSideBarNotebook = GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook);
  gPreviewSideBarPageNumber = gtk_notebook_append_page(
      gSideBarNotebook, gScrolledWindow, gtk_label_new(_("Preview")));

  gtk_widget_show_all(gScrolledWindow);
  gtk_notebook_set_current_page(gSideBarNotebook, gPreviewSideBarPageNumber);

  gSideBarPreviewPage =
      gtk_notebook_get_nth_page(gSideBarNotebook, gPreviewSideBarPageNumber);

  // signal handlers to update notebook
  gHandleSidebarSwitchPage =
      g_signal_connect(GTK_WIDGET(gSideBarNotebook), "switch-page",
                       G_CALLBACK(on_sidebar_switch_page), nullptr);

  gHandleSidebarShow = g_signal_connect(GTK_WIDGET(gSideBarNotebook), "show",
                                        G_CALLBACK(on_sidebar_show), nullptr);

  wv_apply_settings();

  WEBVIEW_WARN(_("Loading."));

  // preview may need to be updated after a delay on first use
  if (gHandleTimeout == 0) {
    gHandleTimeout = g_timeout_add(2000, update_timeout_callback, nullptr);
  }
  return true;
}

static void preview_cleanup(GeanyPlugin *plugin, gpointer data) {
  // fmt_prefs_deinit();
  gtk_notebook_remove_page(gSideBarNotebook, gPreviewSideBarPageNumber);

  gtk_widget_destroy(gWebView);
  gtk_widget_destroy(gScrolledWindow);

  gtk_widget_destroy(gPreviewMenu);

  g_clear_signal_handler(&gHandleSidebarSwitchPage,
                         GTK_WIDGET(gSideBarNotebook));
  g_clear_signal_handler(&gHandleSidebarShow, GTK_WIDGET(gSideBarNotebook));
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
        gtk_widget_is_visible(GTK_WIDGET(gSideBarNotebook))) {
      gtk_notebook_set_current_page(gSideBarNotebook,
                                    gPreviewSideBarPageNumber);
      GtkWidget *page = gtk_notebook_get_nth_page(gSideBarNotebook,
                                                  gPreviewSideBarPageNumber);
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
  std::string command =
      R"(xdg-option ")" +
      cstr_assign_free(g_build_filename(geany_data->app->configdir, "plugins",
                                        "preview", nullptr)) +
      R"(")";

  if (system(command.c_str())) {
    // ignore return value
  }
}

static void on_pref_edit_config(GtkWidget *self, GtkWidget *dialog) {
  open_settings();
  static std::string conf_fn =
      cstr_assign_free(g_build_filename(geany_data->app->configdir, "plugins",
                                        "preview", "preview.conf", nullptr));
  GeanyDocument *doc =
      document_open_file(conf_fn.c_str(), false, nullptr, nullptr);
  document_reload_force(doc, nullptr);

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
static std::string replace_extension(std::string const &fn,
                                     std::string const &ext) {
  std::string fn_noext = cstr_assign_free(
      g_filename_from_utf8(fn.c_str(), -1, nullptr, nullptr, nullptr));
  char *dot = strrchr(fn_noext.data(), '.');
  if (dot != nullptr) {
    *dot = '\0';
  }
  std::string new_fn = fn_noext.data() + ext;
  return new_fn;
}

// from markdown plugin
static void on_menu_export_html(GtkWidget *self, GtkWidget *dialog) {
  GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));

  GtkWidget *save_dialog = gtk_file_chooser_dialog_new(
      _("Save As HTML"), GTK_WINDOW(geany_data->main_widgets->window),
      GTK_FILE_CHOOSER_ACTION_SAVE, _("Cancel"), GTK_RESPONSE_CANCEL, _("Save"),
      GTK_RESPONSE_ACCEPT, nullptr);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(save_dialog),
                                                 true);

  std::string fn = replace_extension(DOC_FILENAME(doc), ".html");
  if (g_file_test(fn.c_str(), G_FILE_TEST_EXISTS)) {
    // If the file exists, GtkFileChooser will change to the correct
    // directory and show the base name as a suggestion.
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(save_dialog), fn.c_str());
  } else {
    // If the file doesn't exist, change the directory and give a
    // suggested name for the file, since GtkFileChooser won't do it.
    std::string dn = cstr_assign_free(g_path_get_dirname(fn.c_str()));
    std::string bn = cstr_assign_free(g_path_get_basename(fn.c_str()));
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(save_dialog),
                                        dn.c_str());
    std::string utf8_name = cstr_assign_free(
        g_filename_to_utf8(bn.c_str(), -1, nullptr, nullptr, nullptr));
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog),
                                      utf8_name.c_str());
  }

  // add file type filters to the chooser
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, _("HTML Files"));
  gtk_file_filter_add_mime_type(filter, "text/html");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, _("All Files"));
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

  bool saved = false;
  while (!saved &&
         gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT) {
    std::string html = update_preview(true);
    fn = cstr_assign_free(
        gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog)));
    if (!file_set_contents(fn.c_str(), html)) {
      dialogs_show_msgbox(GTK_MESSAGE_ERROR,
                          _("Failed to export HTML to file."));
    } else {
      saved = true;
    }
  }

  gtk_widget_destroy(save_dialog);
}

/* ********************
 * Sidebar Functions and Callbacks
 */
static void on_sidebar_switch_page(GtkNotebook *nb, GtkWidget *page,
                                   uint page_num, gpointer user_data) {
  if (gPreviewSideBarPageNumber == page_num) {
    gCurrentDocument = nullptr;
    update_preview(false);
  }
}

static void on_sidebar_show(GtkNotebook *nb, gpointer user_data) {
  if (gtk_notebook_get_current_page(nb) == gPreviewSideBarPageNumber) {
    gCurrentDocument = nullptr;
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
  int temp = jsc_value_to_int32(value);
  if (g_scrollY->len < idx) {
    g_array_insert_val(g_scrollY, idx, temp);
  } else {
    // if (temp > 0) {
    int *scrollY = &g_array_index(g_scrollY, int, idx);
    *scrollY = temp;
    //}
  }

  webkit_javascript_result_unref(js_result);
}

static void wv_save_position() {
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(gWebView), "window.scrollY",
                                 nullptr, wv_save_position_callback, nullptr);
}

static void wv_apply_settings() {
  webkit_user_content_manager_remove_all_style_sheets(gWebViewContentManager);

  webkit_settings_set_default_font_family(gWebViewSettings,
                                          settings.default_font_family);

  // attach headers css
  std::string css_fn = cstr_assign_free(
      find_copy_css("preview-headers.css", PREVIEW_CSS_HEADERS));

  if (!css_fn.empty()) {
    std::string contents = file_get_contents(css_fn);
    if (!contents.empty()) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents.c_str(), WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, nullptr, nullptr);
      webkit_user_content_manager_add_style_sheet(gWebViewContentManager,
                                                  stylesheet);
      webkit_user_style_sheet_unref(stylesheet);
    }
  }

  // attach extra_css
  css_fn = cstr_assign_free(find_css(settings.extra_css));
  if (css_fn.empty()) {
    if (strcmp("dark.css", settings.extra_css) == 0) {
      css_fn =
          cstr_assign_free(find_copy_css(settings.extra_css, PREVIEW_CSS_DARK));
    } else if (strcmp("invert.css", settings.extra_css) == 0) {
      css_fn = cstr_assign_free(
          find_copy_css(settings.extra_css, PREVIEW_CSS_INVERT));
    }
  }

  if (!css_fn.empty()) {
    std::string contents = file_get_contents(css_fn);
    if (!contents.empty()) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents.c_str(), WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, nullptr, nullptr);
      webkit_user_content_manager_add_style_sheet(gWebViewContentManager,
                                                  stylesheet);
      webkit_user_style_sheet_unref(stylesheet);
    }
  }
}

static void wv_load_position() {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return;
  }
  int idx = document_get_notebook_page(doc);

  static std::string script;
  if (gSnippetActive) {
    script = "window.scrollTo(0, 0.2*document.documentElement.scrollHeight);";
  } else if (g_scrollY->len >= idx) {
    int *temp = &g_array_index(g_scrollY, int, idx);
    script = "window.scrollTo(0, " + std::to_string(*temp) + ");";
  } else {
    return;
  }
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(gWebView), script.c_str(),
                                 nullptr, nullptr, nullptr);
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
  gHandleTimeout = 0;
  return false;
}

static std::string update_preview(bool const bGetContents) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return nullptr;
  }

  if (doc != gCurrentDocument) {
    gCurrentDocument = doc;
    set_filetype();
    set_snippets();
  }

  // don't use snippets when exporting html
  if (bGetContents) {
    gSnippetActive = false;
  }

  // save scroll position and set callback if needed
  wv_save_position();
  if (gHandleLoadFinished == 0) {
    gHandleLoadFinished =
        g_signal_connect_swapped(WEBKIT_WEB_VIEW(gWebView), "load-changed",
                                 G_CALLBACK(wv_loading_callback), nullptr);
  }
  std::string uri =
      cstr_assign_free(g_filename_to_uri(DOC_FILENAME(doc), nullptr, nullptr));
  if (uri.empty() || uri.length() < 7 ||
      strncmp(uri.c_str(), "file://", 7) != 0) {
    uri = "file:///tmp/blank.html";
  }

  std::string work_dir =
      cstr_assign_free(g_path_get_dirname(DOC_FILENAME(doc)));
  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  std::string strPlain;
  std::string strOutput;
  std::string strHead;
  std::string strBody;

  // extract snippet for large document
  int position = 0;
  int line = sci_get_current_line(doc->editor->sci);

  int length = sci_get_length(doc->editor->sci);
  if (gSnippetActive) {
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

    strBody =
        cstr_assign_free(sci_get_contents_range(doc->editor->sci, start, end));
  } else {
    strBody = text;
  }

  // check whether need to split head and body
  bool has_header = false;
  try {
    // check if first line matches header format
    static const std::regex re_has_header(R"(^[^\s:]+:\s)");
    if (std::regex_search(strBody, re_has_header)) {
      has_header = true;
    }
  } catch (std::regex_error &e) {
    fprintf(stderr, "regex error in header check\n");
    print_regex_error(e);
  }

  // get format and split head/body
  if (has_header) {
    // split head and body
    if (gFileType != PREVIEW_FILETYPE_FOUNTAIN) {
      std::vector<std::string> lines;
      lines = split_lines(strBody);
      strBody.clear();

      for (auto line : lines) {
        if (has_header) {
          if (line != "") {
            strHead.append(line + "\n");
          } else {
            has_header = false;
            strBody.append(line + "\n");
          }
        } else {
          strBody.append(line + "\n");
        }
      }
    }

    // get format header to set filetype
    try {
      if (!strHead.empty()) {
        static const std::regex re_format(
            R"((content-type|format):\s*([^\n]*))",
            std::regex_constants::icase);
        std::smatch format_match;
        if (std::regex_search(strHead, format_match, re_format)) {
          if (format_match.size() > 2) {
            std::string strFormat = format_match[2];
            gFileType = get_filetype(strFormat.c_str());
          }
        }
      }
    } catch (std::regex_error &e) {
      printf("regex error in format header\n");
      print_regex_error(e);
    }
  }

  switch (gFileType) {
    case PREVIEW_FILETYPE_HTML:
      if (strcmp("disable", settings.html_processor) == 0) {
        strPlain = _("Preview of HTML documents has been disabled.");
      } else if (SUBSTR("pandoc", settings.html_processor))
        strOutput = pandoc(work_dir.c_str(), strBody.c_str(), "html");
      else {
        strOutput = strBody;
      }
      break;
    case PREVIEW_FILETYPE_MARKDOWN:
      if (strcmp("disable", settings.markdown_processor) == 0) {
        strPlain = _("Preview of Markdown documents has been disabled.");
      } else if (SUBSTR("pandoc", settings.markdown_processor))
        strOutput =
            pandoc(work_dir.c_str(), strBody.c_str(), settings.pandoc_markdown);
      else {
        strOutput = cstr_assign_free(
            cmark_markdown_to_html(strBody.c_str(), strBody.length(), 0));
        std::string css_fn = cstr_assign_free(
            find_copy_css("markdown.css", PREVIEW_CSS_MARKDOWN));
        if (!css_fn.empty()) {
          strOutput =
              "<html><head><link rel='stylesheet' "
              "type='text/css' href='file://" +
              css_fn + "'></head><body>" + strOutput + "</body></html>";
        }
      }
      break;
    case PREVIEW_FILETYPE_ASCIIDOC:
      if (strcmp("disable", settings.asciidoc_processor) == 0) {
        strPlain = _("Preview of AsciiDoc documents has been disabled.");
      } else {
        strOutput =
            cstr_assign_free(asciidoctor(work_dir.c_str(), strBody.c_str()));
      }
      break;
    case PREVIEW_FILETYPE_DOCBOOK:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), "docbook"));
      break;
    case PREVIEW_FILETYPE_LATEX:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "latex"));
      break;
    case PREVIEW_FILETYPE_REST:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "rst"));
      break;
    case PREVIEW_FILETYPE_TXT2TAGS:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "t2t"));
      break;
    case PREVIEW_FILETYPE_GFM:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "gfm"));
      break;
    case PREVIEW_FILETYPE_FOUNTAIN:
      if (strcmp("disable", settings.fountain_processor) == 0) {
        strPlain = _("Preview of Fountain screenplays has been disabled.");
      } else if (strcmp("screenplain", settings.fountain_processor) == 0) {
        strOutput = cstr_assign_free(
            screenplain(work_dir.c_str(), strBody.c_str(), "html"));
      } else {
        std::string css_fn = cstr_assign_free(
            find_copy_css("fountain.css", PREVIEW_CSS_FOUNTAIN));
        strOutput = Fountain::ftn2xml(strBody, css_fn);
      }
      break;
    case PREVIEW_FILETYPE_TEXTILE:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), "textile"));
      break;
    case PREVIEW_FILETYPE_DOKUWIKI:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), "dokuwiki"));
      break;
    case PREVIEW_FILETYPE_TIKIWIKI:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), "tikiwiki"));
      break;
    case PREVIEW_FILETYPE_VIMWIKI:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), "vimwiki"));
      break;
    case PREVIEW_FILETYPE_TWIKI:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "twiki"));
      break;
    case PREVIEW_FILETYPE_MEDIAWIKI:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), "mediawiki"));
      break;
    case PREVIEW_FILETYPE_WIKI:
      strOutput = cstr_assign_free(
          pandoc(work_dir.c_str(), strBody.c_str(), settings.wiki_default));
      break;
    case PREVIEW_FILETYPE_MUSE:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "muse"));
      break;
    case PREVIEW_FILETYPE_ORG:
      strOutput =
          cstr_assign_free(pandoc(work_dir.c_str(), strBody.c_str(), "org"));
      break;
    case PREVIEW_FILETYPE_PLAIN:
    case PREVIEW_FILETYPE_EMAIL: {
      strBody = "<pre>" + strBody + "</pre>";
      strOutput = strBody;
    } break;
    case PREVIEW_FILETYPE_NONE:
    default:
      strPlain =
          "Unable to process type: " + std::string{doc->file_type->name} +
          std::string{doc->encoding} + ".";
      break;
  }

  std::string contents;

  if (!strPlain.empty()) {
    WEBVIEW_WARN(strPlain.c_str());

    if (bGetContents) {
      contents = strPlain;
    }
  } else {
    if (strHead != "" && strHead != "\n") {
      try {
        if (!strHead.empty()) {
          if (SUBSTR("<body", strOutput.c_str())) {
            static const std::regex re_body(R"((<body[^>]*>))",
                                            std::regex_constants::icase);
            strHead = "$1\n<pre class='geany_preview_headers'>" + strHead +
                      "</pre>\n";
            strOutput = std::regex_replace(strOutput, re_body, strHead);
          } else {
            strOutput = "<pre class='geany_preview_headers'>" + strHead +
                        "</pre>\n" + strOutput;
          }
        }
      } catch (std::regex_error &e) {
        printf("regex error in format header\n");
        print_regex_error(e);
      }
    }

    // output to webview
    webkit_web_context_clear_cache(gWebViewContext);
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(gWebView), strOutput.c_str(),
                              uri.c_str());

    if (bGetContents) {
      contents = strOutput;
    }
  }

  // restore scroll position
  wv_load_position();

  return contents;
}

static PreviewFileType get_filetype(std::string const &fn) {
  if (fn.empty()) {
    return PREVIEW_FILETYPE_NONE;
  }

  std::string strFormat =
      to_lower(cstr_assign_free(g_path_get_basename(fn.c_str())));

  if (strFormat.empty()) {
    return PREVIEW_FILETYPE_NONE;
  }

  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    return PREVIEW_FILETYPE_NONE;
  }

  PreviewFileType filetype = PREVIEW_FILETYPE_NONE;

  if (SUBSTR("gfm", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_GFM;
  } else if (SUBSTR("fountain", strFormat.c_str()) ||
             SUBSTR("spmd", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_FOUNTAIN;
  } else if (SUBSTR("textile", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_TEXTILE;
  } else if (SUBSTR("txt", strFormat.c_str()) ||
             SUBSTR("plain", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_PLAIN;
  } else if (SUBSTR("eml", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_EMAIL;
  } else if (SUBSTR("wiki", strFormat.c_str())) {
    if (strcmp("disable", settings.wiki_default) == 0) {
      filetype = PREVIEW_FILETYPE_NONE;
    } else if (SUBSTR("dokuwiki", strFormat.c_str())) {
      filetype = PREVIEW_FILETYPE_DOKUWIKI;
    } else if (SUBSTR("tikiwiki", strFormat.c_str())) {
      filetype = PREVIEW_FILETYPE_TIKIWIKI;
    } else if (SUBSTR("vimwiki", strFormat.c_str())) {
      filetype = PREVIEW_FILETYPE_VIMWIKI;
    } else if (SUBSTR("twiki", strFormat.c_str())) {
      filetype = PREVIEW_FILETYPE_TWIKI;
    } else if (SUBSTR("mediawiki", strFormat.c_str()) ||
               SUBSTR("wikipedia", strFormat.c_str())) {
      filetype = PREVIEW_FILETYPE_MEDIAWIKI;
    } else {
      filetype = PREVIEW_FILETYPE_WIKI;
    }
  } else if (SUBSTR("muse", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_MUSE;
  } else if (SUBSTR("org", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_ORG;
  } else if (SUBSTR("html", strFormat.c_str())) {
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_HTML]);
    filetype = PREVIEW_FILETYPE_HTML;
  } else if (SUBSTR("markdown", strFormat.c_str())) {
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_MARKDOWN]);
    filetype = PREVIEW_FILETYPE_MARKDOWN;
  } else if (SUBSTR("asciidoc", strFormat.c_str())) {
    filetype = PREVIEW_FILETYPE_ASCIIDOC;
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_ASCIIDOC]);
  } else if (SUBSTR("docbook", strFormat.c_str())) {
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_DOCBOOK]);
    filetype = PREVIEW_FILETYPE_DOCBOOK;
  } else if (SUBSTR("latex", strFormat.c_str())) {
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_LATEX]);
    filetype = PREVIEW_FILETYPE_LATEX;
  } else if (SUBSTR("rest", strFormat.c_str()) ||
             SUBSTR("restructuredtext", strFormat.c_str())) {
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_REST]);
    filetype = PREVIEW_FILETYPE_REST;
  } else if (SUBSTR("txt2tags", strFormat.c_str()) ||
             SUBSTR("t2t", strFormat.c_str())) {
    document_set_filetype(doc, filetypes[GEANY_FILETYPES_TXT2TAGS]);
    filetype = PREVIEW_FILETYPE_TXT2TAGS;
  }

  return filetype;
}

static inline void set_filetype() {
  GeanyDocument *doc = document_get_current();

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_HTML:
      gFileType = PREVIEW_FILETYPE_HTML;
      break;
    case GEANY_FILETYPES_MARKDOWN:
      gFileType = PREVIEW_FILETYPE_MARKDOWN;
      break;
    case GEANY_FILETYPES_ASCIIDOC:
      gFileType = PREVIEW_FILETYPE_ASCIIDOC;
      break;
    case GEANY_FILETYPES_DOCBOOK:
      gFileType = PREVIEW_FILETYPE_DOCBOOK;
      break;
    case GEANY_FILETYPES_LATEX:
      gFileType = PREVIEW_FILETYPE_LATEX;
      break;
    case GEANY_FILETYPES_REST:
      gFileType = PREVIEW_FILETYPE_REST;
      break;
    case GEANY_FILETYPES_TXT2TAGS:
      gFileType = PREVIEW_FILETYPE_TXT2TAGS;
      break;
    case GEANY_FILETYPES_NONE:
      if (!settings.extended_types) {
        gFileType = PREVIEW_FILETYPE_NONE;
        break;
      } else {
        gFileType = get_filetype(DOC_FILENAME(doc));
      }
      break;
    default:
      gFileType = PREVIEW_FILETYPE_NONE;
      break;
  }
}

static inline void set_snippets() {
  GeanyDocument *doc = document_get_current();

  int length = sci_get_length(doc->editor->sci);
  if (length > settings.snippet_trigger) {
    gSnippetActive = true;
  } else {
    gSnippetActive = false;
  }

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_ASCIIDOC:
      if (!settings.snippet_asciidoctor) {
        gSnippetActive = false;
      }
      break;
    case GEANY_FILETYPES_HTML:
      if (!settings.snippet_html) {
        gSnippetActive = false;
      }
      break;
    case GEANY_FILETYPES_MARKDOWN:
      if (!settings.snippet_markdown) {
        gSnippetActive = false;
      }
      break;
    case GEANY_FILETYPES_NONE: {
      std::string strFormat =
          to_lower(cstr_assign_free(g_path_get_basename(DOC_FILENAME(doc))));
      if (!strFormat.empty()) {
        if (SUBSTR("fountain", strFormat.c_str()) ||
            SUBSTR("spmd", strFormat.c_str())) {
          if (!settings.snippet_screenplain) {
            gSnippetActive = false;
          }
        }
      }
    }  // no break; need to check pandoc setting
    case GEANY_FILETYPES_LATEX:
    case GEANY_FILETYPES_DOCBOOK:
    case GEANY_FILETYPES_REST:
    case GEANY_FILETYPES_TXT2TAGS:
    default:
      if (!settings.snippet_pandoc) {
        gSnippetActive = false;
      }
      break;
  }
}

/* ********************
 * Geany Signal Callbacks
 */

static bool on_editor_notify(GObject *obj, GeanyEditor *editor,
                             SCNotification *notif, gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return false;
  }

  int length = sci_get_length(doc->editor->sci);

  bool need_update = false;
  if (notif->nmhdr.code == SCN_UPDATEUI && gSnippetActive &&
      (notif->updated & (SC_UPDATE_CONTENT | SC_UPDATE_SELECTION))) {
    need_update = true;
  }

  if (notif->nmhdr.code == SCN_MODIFIED && notif->length > 0) {
    if (notif->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      need_update = true;
    }
  }

  if (gtk_notebook_get_current_page(gSideBarNotebook) !=
          gPreviewSideBarPageNumber ||
      !gtk_widget_is_visible(GTK_WIDGET(gSideBarNotebook))) {
    // no updates when preview pane is hidden
    return false;
  }

  if (need_update && gHandleTimeout == 0) {
    if (doc->file_type->id != GEANY_FILETYPES_ASCIIDOC &&
        doc->file_type->id != GEANY_FILETYPES_NONE) {
      // delay for faster programs
      double _tt = (double)length * settings.size_factor_fast;
      int timeout = (int)_tt > settings.update_interval_fast
                        ? (int)_tt
                        : settings.update_interval_fast;
      gHandleTimeout = g_timeout_add(timeout, update_timeout_callback, nullptr);
    } else {
      // delay for slower programs
      double _tt = (double)length * settings.size_factor_slow;
      int timeout = (int)_tt > settings.update_interval_slow
                        ? (int)_tt
                        : settings.update_interval_slow;
      gHandleTimeout = g_timeout_add(timeout, update_timeout_callback, nullptr);
    }
  }

  return false;
}

static void on_document_signal(GObject *obj, GeanyDocument *doc,
                               gpointer user_data) {
  if (gtk_notebook_get_current_page(gSideBarNotebook) !=
          gPreviewSideBarPageNumber ||
      !gtk_widget_is_visible(GTK_WIDGET(gSideBarNotebook))) {
    // no updates when preview pane is hidden
    return;
  }

  gCurrentDocument = nullptr;
  update_preview(false);
}
