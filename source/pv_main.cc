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

#include "pv_main.h"

#include "auxiliary.h"
#include "fountain.h"
#include "pv_formats.h"
#include "pv_settings.h"

#define WEBVIEW_WARN(msg) \
  webkit_web_view_load_plain_text(WEBKIT_WEB_VIEW(gWebView), (msg))

GeanyPlugin *geany_plugin = nullptr;
GeanyData *geany_data = nullptr;

PreviewSettings *gSettings = nullptr;

namespace {  // globals

GtkWidget *gPreviewMenu = nullptr;
GtkWidget *gScrolledWindow = nullptr;

GtkWidget *gWebView = nullptr;
WebKitSettings *gWebViewSettings = nullptr;
WebKitWebContext *gWebViewContext = nullptr;
WebKitUserContentManager *gWebViewContentManager = nullptr;

bool gSnippetActive = false;
int gPreviewSideBarPageNumber = 0;
std::vector<int> gScrollY;

ulong gHandleLoadFinished = 0;
ulong gHandleTimeout = 0;
ulong gHandleSidebarSwitchPage = 0;
ulong gHandleSidebarShow = 0;

GeanyDocument *gCurrentDocument = nullptr;

PreviewFileType gFileType = PREVIEW_FILETYPE_NONE;
GtkNotebook *gSideBarNotebook = nullptr;
GtkWidget *gSideBarPreviewPage = nullptr;

}  // namespace

namespace {  // webview and preview pane functions

void wv_save_position_callback(GObject *object, GAsyncResult *result,
                               gpointer user_data) {
  GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));

  std::size_t idx = document_get_notebook_page(doc);

  JSCValue *value = nullptr;
  GError *error = nullptr;

  value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object),
                                                     result, &error);
  if (!value) {
    g_warning(_("Error running javascript: %s"), error->message);
    GERROR_FREE(error);
    return;
  }

  int temp = jsc_value_to_int32(value);
  if (gScrollY.size() <= idx) {
    gScrollY.resize(idx + 50, 0);
  }
  if (temp > 0) {
    gScrollY[idx] = temp;
  }
}

void wv_save_position() {
  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(gWebView), "window.scrollY", -1, nullptr, nullptr,
      nullptr, wv_save_position_callback, nullptr);
}

void wv_apply_settings() {
  webkit_user_content_manager_remove_all_style_sheets(gWebViewContentManager);

  webkit_settings_set_default_font_family(
      gWebViewSettings, gSettings->default_font_family.c_str());

  // attach headers css
  std::string css_fn = find_copy_css("preview.css", PREVIEW_CSS_HEADERS);

  if (!css_fn.empty()) {
    std::string contents = file_get_contents(css_fn);
    if (!contents.empty()) {
      WebKitUserStyleSheet *stylesheet = webkit_user_style_sheet_new(
          contents.c_str(), WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_AUTHOR, nullptr, nullptr);
      webkit_user_content_manager_add_style_sheet(gWebViewContentManager,
                                                  stylesheet);
    }
  }

  // attach extra_css
  css_fn = find_css(gSettings->extra_css);
  if (css_fn.empty()) {
    if (gSettings->extra_css == "extra-media.css") {
      css_fn = find_copy_css(gSettings->extra_css, PREVIEW_CSS_EXTRA_MEDIA);
    } else if (gSettings->extra_css == "extra-dark.css") {
      css_fn = find_copy_css(gSettings->extra_css, PREVIEW_CSS_EXTRA_DARK);
    } else if (gSettings->extra_css == "extra-invert.css") {
      css_fn = find_copy_css(gSettings->extra_css, PREVIEW_CSS_EXTRA_INVERT);
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
    }
  }
}

void wv_load_position() {
  GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));

  std::size_t idx = document_get_notebook_page(doc);

  static std::string script;
  if (gSnippetActive) {
    script =
        "if (document.documentElement.scrollHeight > window.innerHeight ) { "
        "window.scrollTo(0, 0.2*document.documentElement.scrollHeight); }";
  } else if (gScrollY.size() <= idx) {
    gScrollY.resize(idx + 50, 0);
    return;
  } else {
    script = "window.scrollTo(0, " + std::to_string(gScrollY[idx]) + ");";
  }
  webkit_web_view_evaluate_javascript(WEBKIT_WEB_VIEW(gWebView), script.c_str(),
                                      -1, nullptr, nullptr, nullptr, nullptr,
                                      nullptr);
}

void wv_loading_callback(WebKitWebView *web_view, WebKitLoadEvent load_event,
                         gpointer user_data) {
  // don't wait for WEBKIT_LOAD_FINISHED, othewise preview will flicker
  wv_load_position();
}

/* ********************
 * Other Functions
 */

std::string combine_head_body(std::string &strHead, std::string &strOutput) {
  trim_inplace(strHead);
  if (!strHead.empty()) {
    if (strOutput.substr(0, 1000).find("<body") != std::string::npos) {
      try {
        static const std::regex re_body(R"((<body[^>]*>))",
                                        std::regex_constants::icase);
        strHead =
            "$1\n<pre class='geany_preview_headers'>" + strHead + "</pre>\n";
        strOutput = std::regex_replace(strOutput, re_body, strHead);
      } catch (std::regex_error &e) {
        print_regex_error(e, __FILE__, __LINE__);
      }
    } else {
      strOutput = "<pre class='geany_preview_headers'>" + strHead + "</pre>\n" +
                  strOutput;
    }
  }

  if (strOutput.substr(0, 120).find("<!") != std::string::npos &&
      strOutput.substr(0, 500).find("<style") == std::string::npos) {
    if (strOutput.substr(0, 100).find("<head") != std::string::npos) {
      try {
        static const std::regex re_head(R"((<head[^>]*>))",
                                        std::regex_constants::icase);
        strOutput =
            std::regex_replace(strOutput, re_head, "$1\n<style></style>");
      } catch (std::regex_error &e) {
        print_regex_error(e, __FILE__, __LINE__);
      }
    } else {
      int pos = strOutput.find(">");
      strOutput.insert(pos, "<head><style></style></head>");
    }
  }
  return strOutput;
}

bool use_snippets(GeanyDocument *doc) {
  g_return_val_if_fail(DOC_VALID(doc), false);

  const int length = sci_get_length(doc->editor->sci);
  if (length <= gSettings->snippet_trigger) {
    return false;
  }

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_ASCIIDOC:
      if (!gSettings->snippet_asciidoctor) {
        return false;
      }
      break;
    case GEANY_FILETYPES_HTML:
      if (!gSettings->snippet_html) {
        return false;
      }
      break;
    case GEANY_FILETYPES_MARKDOWN:
      if (!gSettings->snippet_markdown) {
        return false;
      }
      break;
    case GEANY_FILETYPES_NONE:
    case GEANY_FILETYPES_LATEX:
    case GEANY_FILETYPES_DOCBOOK:
    case GEANY_FILETYPES_REST:
    case GEANY_FILETYPES_TXT2TAGS:
    default:
      // When Geany build supports Fountain
      if (strcmp(doc->file_type->name, "Fountain") == 0) {
        if (!gSettings->snippet_fountain) {
          return false;
        }
      }
      {  // When Geany build does not support Fountain
        std::string strFormat =
            to_lower(cstr_assign(g_path_get_basename(DOC_FILENAME(doc))));

        if (strFormat.find("fountain") != std::string::npos) {
          if (!gSettings->snippet_fountain) {
            return false;
          }
        }
      }
      // Otherwise Pandoc fallback
      if (!gSettings->snippet_pandoc) {
        return false;
      }
      break;
  }
  return true;
}

PreviewFileType get_filetype_from_string(const std::string &fn) {
  if (fn.empty() || fn == GEANY_STRING_UNTITLED) {
    return get_filetype_from_string(gSettings->default_type);
  }

  std::string strFormat =
      to_lower(cstr_assign(g_path_get_basename(fn.c_str())));

  if (strFormat.empty()) {
    return PREVIEW_FILETYPE_NONE;
  }

  bool bSetDocFileType = false;
  GeanyDocument *doc = document_get_current();
  if (DOC_VALID(doc)) {
    bSetDocFileType = true;
  }

  if (strFormat.find("fountain") != std::string::npos ||
      strFormat.find("spmd") != std::string::npos) {
    if (bSetDocFileType) {
      if (GeanyFiletype *nft = filetypes_lookup_by_name("Fountain")) {
        document_set_filetype(doc, nft);
      }
    }
    return PREVIEW_FILETYPE_FOUNTAIN;
  } else if (strFormat.find("html") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_HTML]);
    }
    return PREVIEW_FILETYPE_HTML;
  } else if (strFormat.find("markdown") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_MARKDOWN]);
    }
    return PREVIEW_FILETYPE_MARKDOWN;
  } else if (strFormat.find("txt") != std::string::npos ||
             strFormat.find("plain") != std::string::npos) {
    return PREVIEW_FILETYPE_PLAIN;
  } else if (strFormat.find("gfm") != std::string::npos) {
    return PREVIEW_FILETYPE_GFM;
  } else if (strFormat.find("eml") != std::string::npos) {
    return PREVIEW_FILETYPE_EMAIL;
  } else if (strFormat.find("asciidoc") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_ASCIIDOC]);
    }
    return PREVIEW_FILETYPE_ASCIIDOC;
  } else if (strFormat.find("latex") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_LATEX]);
    }
    return PREVIEW_FILETYPE_LATEX;
  } else if (strFormat.find("rst") != std::string::npos ||
             strFormat.find("rest") != std::string::npos ||
             strFormat.find("restructuredtext") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_REST]);
    }
    return PREVIEW_FILETYPE_REST;
  } else if (strFormat.find("txt2tags") != std::string::npos ||
             strFormat.find("t2t") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_TXT2TAGS]);
    }
    return PREVIEW_FILETYPE_TXT2TAGS;
  } else if (strFormat.find("textile") != std::string::npos) {
    return PREVIEW_FILETYPE_TEXTILE;
  } else if (strFormat.find("docbook") != std::string::npos) {
    if (bSetDocFileType) {
      document_set_filetype(doc, filetypes[GEANY_FILETYPES_DOCBOOK]);
    }
    return PREVIEW_FILETYPE_DOCBOOK;
  } else if (strFormat.find("wiki") != std::string::npos) {
    if (gSettings->wiki_default == "disable") {
      return PREVIEW_FILETYPE_NONE;
    } else if (strFormat.find("dokuwiki") != std::string::npos) {
      return PREVIEW_FILETYPE_DOKUWIKI;
    } else if (strFormat.find("tikiwiki") != std::string::npos) {
      return PREVIEW_FILETYPE_TIKIWIKI;
    } else if (strFormat.find("vimwiki") != std::string::npos) {
      return PREVIEW_FILETYPE_VIMWIKI;
    } else if (strFormat.find("twiki") != std::string::npos) {
      return PREVIEW_FILETYPE_TWIKI;
    } else if (strFormat.find("mediawiki") != std::string::npos ||
               strFormat.find("wikipedia") != std::string::npos) {
      return PREVIEW_FILETYPE_MEDIAWIKI;
    } else {
      return PREVIEW_FILETYPE_WIKI;
    }
  } else if (strFormat.find("muse") != std::string::npos) {
    return PREVIEW_FILETYPE_MUSE;
  } else if (strFormat.find("org") != std::string::npos) {
    return PREVIEW_FILETYPE_ORG;
  }

  return PREVIEW_FILETYPE_NONE;
}

PreviewFileType get_filetype(GeanyDocument *doc) {
  g_return_val_if_fail(DOC_VALID(doc), PREVIEW_FILETYPE_NONE);

  switch (doc->file_type->id) {
    case GEANY_FILETYPES_HTML:
      return PREVIEW_FILETYPE_HTML;
      break;
    case GEANY_FILETYPES_MARKDOWN:
      return PREVIEW_FILETYPE_MARKDOWN;
      break;
    case GEANY_FILETYPES_ASCIIDOC:
      return PREVIEW_FILETYPE_ASCIIDOC;
      break;
    case GEANY_FILETYPES_DOCBOOK:
      return PREVIEW_FILETYPE_DOCBOOK;
      break;
    case GEANY_FILETYPES_LATEX:
      return PREVIEW_FILETYPE_LATEX;
      break;
    case GEANY_FILETYPES_REST:
      return PREVIEW_FILETYPE_REST;
      break;
    case GEANY_FILETYPES_TXT2TAGS:
      return PREVIEW_FILETYPE_TXT2TAGS;
      break;
    case GEANY_FILETYPES_NONE:
    case GEANY_FILETYPES_XML:
      if (!gSettings->extended_types) {
        return PREVIEW_FILETYPE_NONE;
      } else {
        return get_filetype_from_string(DOC_FILENAME(doc));
      }
      break;
    default:
      if (strcmp(doc->file_type->name, "Fountain") == 0) {
        return PREVIEW_FILETYPE_FOUNTAIN;
      }

      return PREVIEW_FILETYPE_NONE;
      break;
  }
  return PREVIEW_FILETYPE_NONE;
}

std::string update_preview(const bool bGetContents) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return {};
  }

  if (doc != gCurrentDocument) {
    gCurrentDocument = doc;
    gFileType = get_filetype(doc);
    gSnippetActive = use_snippets(doc);
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
      cstr_assign(g_filename_to_uri(DOC_FILENAME(doc), nullptr, nullptr));
  if (uri.empty() || uri.length() < 7 ||
      strncmp(uri.c_str(), "file://", 7) != 0) {
    uri = "file:///tmp/blank.html";
  }

  std::string work_dir = cstr_assign(g_path_get_dirname(DOC_FILENAME(doc)));
  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);
  std::string strPlain;   // used to output errors
  std::string strOutput;  // used to html output after processing
  std::string strHead;    // holds email-style headers
  std::string strBody;    // temporarily holds body before processing

  // extract snippet for large documents
  {
    int position = 0;
    int nLine = sci_get_current_line(doc->editor->sci);

    int length = sci_get_length(doc->editor->sci);
    if (gSnippetActive) {
      if (nLine > 0) {
        position = sci_get_position_from_line(doc->editor->sci, nLine);
      }
      int start = 0;
      int end = 0;
      int amount = gSettings->snippet_window / 3;
      // get beginning and end positions for snippet
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

      int start_line = scintilla_send_message(
          doc->editor->sci, SCI_LINEFROMPOSITION, start, start);
      start = scintilla_send_message(doc->editor->sci, SCI_POSITIONFROMLINE,
                                     start_line, start_line);

      int end_line = scintilla_send_message(doc->editor->sci,
                                            SCI_LINEFROMPOSITION, end, end);
      end = scintilla_send_message(doc->editor->sci, SCI_GETLINEENDPOSITION,
                                   end_line, end_line);

      strBody = std::string(text + start, end - start);
    } else {
      strBody = text;
    }
  }

  // check whether need to split head and body
  bool has_header = false;
  try {
    // check if first line matches header format
    static const std::regex re_has_header(R"(^[^\s:]+:[ \t]+\S)");
    if (std::regex_search(strBody, re_has_header)) {
      has_header = true;
    }
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  // get format and split head/body
  if (has_header) {
    // split head and body
    if (gFileType != PREVIEW_FILETYPE_FOUNTAIN) {
      std::vector<std::string> lines = split_lines(strBody);
      strBody.clear();

      for (auto line : lines) {
        if (has_header) {
          if (line != "") {
            strHead.append(line + '\n');
          } else {
            has_header = false;
            strBody.append(line + '\n');
          }
        } else {
          strBody.append(line + '\n');
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
            gFileType = get_filetype_from_string(strFormat);
          }
        }
      }
    } catch (std::regex_error &e) {
      print_regex_error(e, __FILE__, __LINE__);
    }
  }

  switch (gFileType) {
    case PREVIEW_FILETYPE_HTML:
      if (gSettings->processor_html == "disable") {
        strPlain = _("Preview of HTML documents has been disabled.");
      } else if (gSettings->processor_html.find("pandoc") !=
                 std::string::npos) {
        strOutput = pandoc(work_dir, strBody, "html");
      } else {
        strOutput = strBody;
      }
      break;
    case PREVIEW_FILETYPE_MARKDOWN:
      if (gSettings->processor_markdown == "disable") {
        strPlain = _("Preview of Markdown documents has been disabled.");
      } else if (gSettings->processor_markdown.find("pandoc") !=
                 std::string::npos) {
        strOutput = pandoc(work_dir, strBody, gSettings->pandoc_markdown);
      } else {
        strOutput = cmark_gfm(strBody);
      }
      break;
    case PREVIEW_FILETYPE_ASCIIDOC:
      if (gSettings->processor_asciidoc == "disable") {
        strPlain = _("Preview of AsciiDoc documents has been disabled.");
      } else if (gSettings->processor_asciidoc == "asciidoc") {
        strOutput = asciidoc(work_dir, strBody);
      } else {
        strOutput = asciidoctor(work_dir, strBody);
      }
      break;
    case PREVIEW_FILETYPE_DOCBOOK:
      strOutput = pandoc(work_dir, strBody, "docbook");
      break;
    case PREVIEW_FILETYPE_LATEX:
      strOutput = pandoc(work_dir, strBody, "latex");
      break;
    case PREVIEW_FILETYPE_REST:
      strOutput = pandoc(work_dir, strBody, "rst");
      break;
    case PREVIEW_FILETYPE_TXT2TAGS:
      strOutput = pandoc(work_dir, strBody, "t2t");
      break;
    case PREVIEW_FILETYPE_GFM:
      strOutput = pandoc(work_dir, strBody, "gfm");
      break;
    case PREVIEW_FILETYPE_FOUNTAIN:
      if (gSettings->processor_fountain == "disable") {
        strPlain = _("Preview of Fountain screenplays has been disabled.");
      } else {
        std::string css_fn =
            find_copy_css("fountain.css", PREVIEW_CSS_FOUNTAIN);
        strOutput = Fountain::ftn2xml(strBody, css_fn, true);
      }
      break;
    case PREVIEW_FILETYPE_TEXTILE:
      strOutput = pandoc(work_dir, strBody, "textile");
      break;
    case PREVIEW_FILETYPE_DOKUWIKI:
      strOutput = pandoc(work_dir, strBody, "dokuwiki");
      break;
    case PREVIEW_FILETYPE_TIKIWIKI:
      strOutput = pandoc(work_dir, strBody, "tikiwiki");
      break;
    case PREVIEW_FILETYPE_VIMWIKI:
      strOutput = pandoc(work_dir, strBody, "vimwiki");
      break;
    case PREVIEW_FILETYPE_TWIKI:
      strOutput = pandoc(work_dir, strBody, "twiki");
      break;
    case PREVIEW_FILETYPE_MEDIAWIKI:
      strOutput = pandoc(work_dir, strBody, "mediawiki");
      break;
    case PREVIEW_FILETYPE_WIKI:
      strOutput = pandoc(work_dir, strBody, gSettings->wiki_default);
      break;
    case PREVIEW_FILETYPE_MUSE:
      strOutput = pandoc(work_dir, strBody, "muse");
      break;
    case PREVIEW_FILETYPE_ORG:
      strOutput = pandoc(work_dir, strBody, "org");
      break;
    case PREVIEW_FILETYPE_PLAIN:
    case PREVIEW_FILETYPE_EMAIL: {
      if (!strHead.empty()) {
        strBody = "<pre style='white-space: pre-wrap;'>" +
                  encode_entities_inplace(strBody) + "</pre>";
        strOutput = strBody;
      }
    } break;
    case PREVIEW_FILETYPE_NONE:
    default:
      break;
  }

  if (strOutput.empty() && strPlain.empty()) {
    strPlain = std::string{doc->file_type->name} + ", " +
               std::string{doc->encoding} + ".";
  }

  std::string contents;

  if (!strPlain.empty()) {
    WEBVIEW_WARN(strPlain.c_str());

    if (bGetContents) {
      contents = strPlain;
    }
  } else {
    strOutput = combine_head_body(strHead, strOutput);

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

gboolean update_timeout_callback(gpointer user_data) {
  update_preview(false);
  gHandleTimeout = 0;
  return false;
}
}  // namespace

namespace {  // Sidebar Functions and Callbacks
void preview_sidebar_switch_page(GtkNotebook *nb, GtkWidget *page, int page_num,
                                 gpointer user_data) {
  if (gPreviewSideBarPageNumber == page_num) {
    gCurrentDocument = nullptr;
    update_preview(false);
  }
}

void preview_sidebar_show(GtkNotebook *nb, gpointer user_data) {
  if (gtk_notebook_get_current_page(nb) == gPreviewSideBarPageNumber) {
    gCurrentDocument = nullptr;
    update_preview(false);
  }
}
}  // namespace

namespace {  // Keybinding and Preferences

void preview_toggle_editor_preview() {
  GeanyDocument *doc = document_get_current();
  if (doc != nullptr) {
    GtkWidget *sci = GTK_WIDGET(doc->editor->sci);
    if (gtk_widget_has_focus(sci) &&
        gtk_widget_is_visible(GTK_WIDGET(gSideBarNotebook))) {
      gtk_notebook_set_current_page(gSideBarNotebook,
                                    gPreviewSideBarPageNumber);
      g_signal_emit_by_name(G_OBJECT(gSideBarNotebook), "grab-focus", nullptr);
      gtk_widget_grab_focus(GTK_WIDGET(gWebView));
    } else {
      g_signal_emit_by_name(G_OBJECT(gSideBarNotebook), "grab-notify", nullptr);
      keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    }
  }
}

void preview_menu_preferences(GtkWidget *self, GtkWidget *dialog) {
  plugin_show_configure(geany_plugin);
}

// from markdown plugin
std::string replace_extension(const std::string &fn, const std::string &ext) {
  if (fn.empty()) {
    return ext;
  }

  char *fn_noext =
      g_filename_from_utf8(fn.c_str(), -1, nullptr, nullptr, nullptr);
  char *dot = strrchr(fn_noext, '.');
  if (dot != nullptr) {
    *dot = '\0';
  }
  std::string new_fn = fn_noext + ext;
  g_free(fn_noext);
  return new_fn;
}

// from markdown plugin
void preview_menu_export_html(GtkWidget *self, GtkWidget *dialog) {
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
    std::string dn = cstr_assign(g_path_get_dirname(fn.c_str()));
    std::string bn = cstr_assign(g_path_get_basename(fn.c_str()));
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(save_dialog),
                                        dn.c_str());
    std::string utf8_name = cstr_assign(
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
    fn = cstr_assign(
        gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog)));
    if (!file_set_contents(fn, html)) {
      dialogs_show_msgbox(GTK_MESSAGE_ERROR,
                          _("Failed to export HTML to file."));
    } else {
      saved = true;
    }
  }

  gtk_widget_destroy(save_dialog);
}

#if ENABLE_EXPORT_PDF

void preview_menu_export_pdf(GtkWidget *self, GtkWidget *dialog) {
  GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));

  // PDF export only supported for Fountain
  gFileType = get_filetype(doc);
  if (gFileType != PREVIEW_FILETYPE_FOUNTAIN) {
    msgwin_status_add("PDF export is available only for Fountain documents.");
    return;
  }

  char *text = (char *)scintilla_send_message(doc->editor->sci,
                                              SCI_GETCHARACTERPOINTER, 0, 0);

  GtkWidget *save_dialog = gtk_file_chooser_dialog_new(
      _("Save As PDF"), GTK_WINDOW(geany_data->main_widgets->window),
      GTK_FILE_CHOOSER_ACTION_SAVE, _("Cancel"), GTK_RESPONSE_CANCEL, _("Save"),
      GTK_RESPONSE_ACCEPT, nullptr);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(save_dialog),
                                                 true);

  std::string fn = replace_extension(DOC_FILENAME(doc), ".pdf");
  if (g_file_test(fn.c_str(), G_FILE_TEST_EXISTS)) {
    // If the file exists, GtkFileChooser will change to the correct
    // directory and show the base name as a suggestion.
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(save_dialog), fn.c_str());
  } else {
    // If the file doesn't exist, change the directory and give a
    // suggested name for the file, since GtkFileChooser won't do it.
    std::string dn = cstr_assign(g_path_get_dirname(fn.c_str()));
    std::string bn = cstr_assign(g_path_get_basename(fn.c_str()));
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(save_dialog),
                                        dn.c_str());
    std::string utf8_name = cstr_assign(
        g_filename_to_utf8(bn.c_str(), -1, nullptr, nullptr, nullptr));
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog),
                                      utf8_name.c_str());
  }

  // add file type filters to the chooser
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, _("PDF Files"));
  gtk_file_filter_add_mime_type(filter, "application/pdf");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, _("All Files"));
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

  bool saved = false;
  while (!saved &&
         gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT) {
    fn = cstr_assign(
        gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog)));

    if (!Fountain::ftn2pdf(fn, text)) {
      dialogs_show_msgbox(GTK_MESSAGE_ERROR,
                          _("Failed to export PDF to file."));
    } else {
      saved = true;
    }
  }

  gtk_widget_destroy(save_dialog);
}

#endif  // ENABLE_EXPORT_PDF

bool preview_key_binding(int key_id) {
  switch (key_id) {
    case PREVIEW_KEY_TOGGLE_EDITOR:
      preview_toggle_editor_preview();
      break;
    case PREVIEW_KEY_EXPORT_HTML:
      preview_menu_export_html(nullptr, nullptr);
      break;

#if ENABLE_EXPORT_PDF
    case PREVIEW_KEY_EXPORT_PDF:
      preview_menu_export_pdf(nullptr, nullptr);
      break;
#endif  // ENABLE_EXPORT_PDF

    default:
      return false;
  }
  return true;
}

}  // namespace

bool preview_editor_notify(GObject *obj, GeanyEditor *editor,
                           SCNotification *notif, gpointer user_data) {
  // no update when update pending
  if (gHandleTimeout) {
    return false;
  }

  // no updates when preview pane is hidden
  if (gtk_notebook_get_current_page(gSideBarNotebook) !=
          gPreviewSideBarPageNumber ||
      !gtk_widget_is_visible(GTK_WIDGET(gSideBarNotebook))) {
    return false;
  }

  // no update if document invalid
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc)) {
    WEBVIEW_WARN(_("Unknown document type."));
    return false;
  }

  int length = sci_get_length(doc->editor->sci);

  // determine whether update is needed
  bool need_update = false;
  if (gSnippetActive && notif->nmhdr.code == SCN_UPDATEUI &&
      (notif->updated & (SC_UPDATE_CONTENT | SC_UPDATE_SELECTION))) {
    need_update = true;
  }

  if (notif->nmhdr.code == SCN_MODIFIED && notif->length > 0) {
    if (notif->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      need_update = true;
    }
  }

  if (!need_update) {
    return false;
  }

  // determine update speed
  enum PreviewUpdateSpeed {
    FAST,
    SLOW,
    SUPER_SLOW,
  } update_speed = SLOW;

  switch (gFileType) {
    case PREVIEW_FILETYPE_NONE:
      update_speed = SUPER_SLOW;
      break;
    case PREVIEW_FILETYPE_FOUNTAIN: {
      if (gSettings->processor_fountain == "disable") {
        update_speed = SUPER_SLOW;
      } else {
        update_speed = FAST;
      }
    } break;
    case PREVIEW_FILETYPE_ASCIIDOC:
      update_speed = SLOW;
      break;

    case PREVIEW_FILETYPE_PLAIN:
    case PREVIEW_FILETYPE_EMAIL:
      update_speed = FAST;
      break;

    case PREVIEW_FILETYPE_GFM:
    case PREVIEW_FILETYPE_HTML: {
      if (gSettings->processor_html == "disable") {
        update_speed = SUPER_SLOW;
      } else {
        update_speed = FAST;
      }
    } break;
    case PREVIEW_FILETYPE_MARKDOWN: {
      if (gSettings->processor_markdown == "disable") {
        update_speed = SUPER_SLOW;
      } else {
        // pandoc or native
        update_speed = FAST;
      }
    } break;

    default:
      if (gSettings->pandoc_disabled) {
        update_speed = SUPER_SLOW;
      } else {
        update_speed = FAST;
      }
      break;
  }

  int timeout = 5 * gSettings->update_interval_slow;
  switch (update_speed) {
    case FAST: {
      // delay for faster programs (native html/markdown/fountain and pandoc)
      double _tt = (double)length * gSettings->size_factor_fast;
      timeout = (int)_tt > gSettings->update_interval_fast
                    ? (int)_tt
                    : gSettings->update_interval_fast;
    } break;
    case SLOW: {
      // delay for slower programs (asciidoctor)
      double _tt = (double)length * gSettings->size_factor_slow;
      timeout = (int)_tt > gSettings->update_interval_slow
                    ? (int)_tt
                    : gSettings->update_interval_slow;
    } break;
    case SUPER_SLOW:
      // slow update when unhandled filetype;
      // timeout is set before switch;
      // needed to catch filetype change
      break;
  }

  gHandleTimeout = g_timeout_add(timeout, update_timeout_callback, nullptr);
  return true;
}

void preview_document_signal(GObject *obj, GeanyDocument *doc,
                             gpointer user_data) {
  gCurrentDocument = nullptr;

  // no updates when preview pane is hidden
  if (gtk_notebook_get_current_page(gSideBarNotebook) !=
          gPreviewSideBarPageNumber ||
      !gtk_widget_is_visible(GTK_WIDGET(gSideBarNotebook))) {
    return;
  }

  // update only when update not already pending
  if (!gHandleTimeout) {
    gHandleTimeout = g_timeout_add(gSettings->update_interval_fast,
                                   update_timeout_callback, nullptr);
  }
}

void preview_pref_save_config(GtkWidget *self, GtkWidget *dialog) {
  gSettings->save();
}

void preview_pref_reload_config(GtkWidget *self, GtkWidget *dialog) {
  gSettings->load();
  // wv_apply_settings();
}

void preview_pref_reset_config(GtkWidget *self, GtkWidget *dialog) {
  gSettings->reset();
}

void preview_pref_edit_config(GtkWidget *self, GtkWidget *dialog) {
  namespace fs = std::filesystem;

  gSettings->load();

  static fs::path conf_fn = fs::path{geany_data->app->configdir} / "plugins" /
                            "preview" / "preview.conf";

  GeanyDocument *doc =
      document_open_file(conf_fn.c_str(), false, nullptr, nullptr);
  document_reload_force(doc, nullptr);

  if (dialog != nullptr) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }
}

void preview_pref_open_config_folder(GtkWidget *self, GtkWidget *dialog) {
  namespace fs = std::filesystem;

  fs::path conf_dir =
      fs::path{geany_data->app->configdir} / "plugins" / "preview";
  std::string command = R"(xdg-open ")" + conf_dir.string() + R"(")";

  if (std::system(command.c_str())) {
    // ignore return value
  }
}

GtkWidget *preview_configure(GeanyPlugin *plugin, GtkDialog *dialog,
                             gpointer data) {
  GtkWidget *box, *btn;
  char *tooltip;

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

  tooltip = g_strdup(_("Save the active settings to the config file."));
  btn = gtk_button_new_with_label(_("Save Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(preview_pref_save_config),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip = g_strdup(
      _("Reload settings from the config file.  May be used "
        "to apply preferences after editing without restarting Geany."));
  btn = gtk_button_new_with_label(_("Reload Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(preview_pref_reload_config),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip =
      g_strdup(_("Delete the current config file and restore the default "
                 "file with explanatory comments."));
  btn = gtk_button_new_with_label(_("Reset Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(preview_pref_reset_config),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip = g_strdup(_("Open the config file in Geany for editing."));
  btn = gtk_button_new_with_label(_("Edit Config"));
  g_signal_connect(btn, "clicked", G_CALLBACK(preview_pref_edit_config),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  tooltip =
      g_strdup(_("Open the config folder in the default file manager.  The "
                 "config folder "
                 "contains the stylesheets, which may be edited."));
  btn = gtk_button_new_with_label(_("Open Config Folder"));
  g_signal_connect(btn, "clicked", G_CALLBACK(preview_pref_open_config_folder),
                   dialog);
  gtk_box_pack_start(GTK_BOX(box), btn, false, false, 3);
  gtk_widget_set_tooltip_text(btn, tooltip);
  g_free(tooltip);

  return box;
}

void preview_cleanup(GeanyPlugin *plugin, gpointer pdata) {
  gSettings->kf_close();
  delete gSettings;

  if (gHandleSidebarSwitchPage) {
    g_clear_signal_handler(&gHandleSidebarSwitchPage,
                           GTK_WIDGET(gSideBarNotebook));
  }
  if (gHandleSidebarShow) {
    g_clear_signal_handler(&gHandleSidebarShow, GTK_WIDGET(gSideBarNotebook));
  }

  gtk_widget_destroy(gPreviewMenu);
  gtk_widget_destroy(gSideBarPreviewPage);
}

gboolean preview_init(GeanyPlugin *plugin, gpointer pdata) {
  geany_plugin = plugin;
  geany_data = plugin->geany_data;

  gScrollY.resize(50, 0);

  gSettings = new PreviewSettings();
  gSettings->initialize();
  gSettings->load();

  // Callbacks
  GEANY_PSC("geany-startup-complete", preview_document_signal);
  GEANY_PSC("editor-notify", preview_editor_notify);
  GEANY_PSC("document-activate", preview_document_signal);
  GEANY_PSC("document-filetype-set", preview_document_signal);
  GEANY_PSC("document-new", preview_document_signal);
  GEANY_PSC("document-open", preview_document_signal);
  GEANY_PSC("document-reload", preview_document_signal);

  // Set keyboard shortcuts
  GeanyKeyGroup *group =
      plugin_set_key_group(geany_plugin, "Preview", PREVIEW_KEY_COUNT,
                           (GeanyKeyGroupCallback)preview_key_binding);

  keybindings_set_item(group, PREVIEW_KEY_TOGGLE_EDITOR, nullptr, 0,
                       GdkModifierType(0), "preview_toggle_editor",
                       _("Toggle focus between editor and preview"), nullptr);

  keybindings_set_item(group, PREVIEW_KEY_EXPORT_HTML, nullptr, 0,
                       GdkModifierType(0), "preview_export_html",
                       _("Export document to HTML"), nullptr);
#if ENABLE_EXPORT_PDF
  keybindings_set_item(group, PREVIEW_KEY_EXPORT_PDF, nullptr, 0,
                       GdkModifierType(0), "preview_export_pdf",
                       _("Export Fountain screenplay to PDF"), nullptr);
#endif  // ENABLE_EXPORT_PDF

  // set up menu
  // GeanyKeyGroup *group;
  GtkWidget *item;

  gPreviewMenu = gtk_menu_item_new_with_label(_("Preview"));
  ui_add_document_sensitive(gPreviewMenu);

  GtkWidget *submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(gPreviewMenu), submenu);

  item = gtk_menu_item_new_with_label(_("Export to HTML..."));
  g_signal_connect(item, "activate", G_CALLBACK(preview_menu_export_html),
                   nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

#if ENABLE_EXPORT_PDF
  item = gtk_menu_item_new_with_label(_("Export to PDF..."));
  g_signal_connect(item, "activate", G_CALLBACK(preview_menu_export_pdf),
                   nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
#endif  // ENABLE_EXPORT_PDF

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Edit Config File"));
  g_signal_connect(item, "activate", G_CALLBACK(preview_pref_edit_config),
                   nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Reload Config File"));
  g_signal_connect(item, "activate", G_CALLBACK(preview_pref_reload_config),
                   nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Open Config Folder"));
  g_signal_connect(item, "activate",
                   G_CALLBACK(preview_pref_open_config_folder), nullptr);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

  item = gtk_menu_item_new_with_label(_("Preferences"));
  g_signal_connect(item, "activate", G_CALLBACK(preview_menu_preferences),
                   nullptr);
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

  // show preview pane
  gtk_widget_show_all(gScrolledWindow);
  gtk_notebook_set_current_page(gSideBarNotebook, gPreviewSideBarPageNumber);

  gSideBarPreviewPage =
      gtk_notebook_get_nth_page(gSideBarNotebook, gPreviewSideBarPageNumber);

  gtk_widget_set_name(GTK_WIDGET(gSideBarPreviewPage),
                      "geany-preview-sidebar-page");

  // signal handlers to update notebook
  gHandleSidebarSwitchPage =
      g_signal_connect(GTK_WIDGET(gSideBarNotebook), "switch-page",
                       G_CALLBACK(preview_sidebar_switch_page), nullptr);

  gHandleSidebarShow =
      g_signal_connect(GTK_WIDGET(gSideBarNotebook), "show",
                       G_CALLBACK(preview_sidebar_show), nullptr);

  wv_apply_settings();

  WEBVIEW_WARN(_("Loading."));

  // preview may need to be updated after a delay on first use
  if (gHandleTimeout == 0) {
    gHandleTimeout = g_timeout_add(gSettings->startup_timeout,
                                   update_timeout_callback, nullptr);
  }

  return true;
}

void geany_load_module(GeanyPlugin *plugin) {
  plugin->info->name = "Preview";
  plugin->info->description = _(
      "Preview pane for HTML, Markdown, and other lightweight markup formats.");
  plugin->info->version = VERSION;
  plugin->info->author = "xiota";
  plugin->funcs->init = preview_init;
  plugin->funcs->configure = preview_configure;
  plugin->funcs->help = nullptr;
  plugin->funcs->cleanup = preview_cleanup;
  plugin->funcs->callbacks = nullptr;

  GEANY_PLUGIN_REGISTER(plugin, 225);
}
