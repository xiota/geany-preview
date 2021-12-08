/*
 * WebView - Preview Plugin for Geany
 * Copyright 2021 xiota
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

#pragma once

#include "tkui_addons.h"

class PreviewWebView {
 public:
  void initialize();

 public:
  bool enable = true;

  std::string desc_startup_timeout{
      _("Timeout before forcing webview update during startup.")};
  int startup_timeout = 350;

  std::string desc_update_interval_slow{
      _("The following option is the minimum number of milliseconds between "
        "calls for slow external programs.  This gives them time to finish "
        "processing.  Use a larger number for slower machines.")};
  int update_interval_slow = 200;

  std::string desc_size_factor_slow{
      _("For large files, a longer delay is needed.  The delay in "
        "milliseconds is the filesize in bytes multiplied by "
        "size_factor_slow.")};
  double size_factor_slow = 0.004;

  std::string desc_update_interval_fast{
      _("Even fast programs may need a delay to finish processing "
        "on slow computers.")};
  int update_interval_fast = 25;
  double size_factor_fast = 0.001;

  std::string desc_processor_html{
      _("The following option controls how HTML documents are processed.\n"
        "  - native* - Send the document directly to the webview.\n"
        "  - pandoc  - Use Pandoc to clean up the document and apply a "
        "stylesheet.\n"
        "  - disable - Turn off HTML previews.")};
  std::string processor_html{"native"};

  std::string desc_processor_markdown{
      _("The following option selects the Markdown processor.\n"
        "  - native* - Use libcmark-gfm (faster)\n"
        "  - pandoc  - Use Pandoc GFM   (applies additional pandoc options)\n"
        "  - disable - Turn off Markdown previews.")};
  std::string processor_markdown{"native"};

  std::string desc_processor_asciidoc{
      _("The following option enables or disables AsciiDoc previews.\n"
        "  - asciidoctor* - Use asciidoctor.\n"
        "  - disable      - Turn off AsciiDoc previews.")};
  std::string processor_asciidoc{"asciidoctor"};

  std::string desc_processor_fountain{
      _("The following option selects the Fountain processor.\n"
        "  - native*      - Use the built-in fountain processor\n"
        "  - screenplain  - Use screenplain (slower)\n"
        "  - disable      - Turn off Fountain screenplay previews")};
  std::string processor_fountain{"native"};

  std::string desc_wiki_default{
      _("The following option selects the format that Pandoc uses to "
        "interpret files with the .wiki extension.  It can also turn off wiki "
        "previews.  Options may be any format from `pandoc "
        "--list-input-formats`: disable, dokuwiki, *mediawiki*, tikiwiki, "
        "twiki, vimwiki")};
  std::string wiki_default{"mediawiki"};

  std::string desc_pandoc_disabled{
      _("The following option disables Pandoc processing.")};
  bool pandoc_disabled = false;

  std::string desc_pandoc_fragment{
      _("The following option controls whether Pandoc produces standalone HTML "
        "files.  If set to true, stylesheets are effectively disabled.")};
  bool pandoc_fragment = false;

  std::string desc_pandoc_toc{
      _("The following option controls whether Pandoc inserts an "
        "auto-generated table of contents at the beginning of HTML "
        "files.  This might help with navigating long documents.")};
  bool pandoc_toc = false;

  std::string desc_pandoc_markdown{
      _("The following option determines how Pandoc interprets Markdown.  It "
        "has *no* effect unless Pandoc is set as the processor_markdown.  "
        "Options may be any format from `pandoc --list-input-formats`: "
        "commonmark, *markdown*, markdown_github, markdown_mmd, "
        "markdown_phpextra, markdown_strict")};
  std::string pandoc_markdown{"markdown"};

  std::string desc_default_font_family{
      _("The following option determines what font family the webview "
        "uses when another font is not specified by a stylesheet.")};
  std::string default_font_family{"serif"};

  std::string desc_extended_types{
      _("The following option controls whether the file extension is used to "
        "detect file types that are unknown to Geany.  This feature is needed "
        "to preview fountain, textile, wiki, muse, and org documents.")};
  bool extended_types = true;

  std::string desc_snippet_window{
      _("Large documents are both slow to process and difficult to navigate.  "
        "The snippet settings allow a small region of interest to be "
        "previewed.  The trigger is the file size (bytes) that activates "
        "snippets.  The window is the size (bytes) around the caret that is "
        "processed.")};
  int snippet_window = 5000;
  int snippet_trigger = 100000;

  std::string desc_snippet_html{
      _("Some document types don't work well with the snippets feature.  The "
        "following options disable (false) and enable (true) them.")};
  bool snippet_html = false;
  bool snippet_markdown = true;
  bool snippet_asciidoctor = true;
  bool snippet_pandoc = true;
  bool snippet_fountain = true;

  std::string desc_extra_css{
      _("Previews are styled with `css` files that correspond to the document "
        "processor and file type.  They are located in "
        "`~/.config/geany/plugins/preview/`\n"
        " Default files may be copied from "
        "`/usr/share/geany-plugins/preview/`\n"
        "\n"
        " Files are created as needed.  To disable, replace with an empty "
        "file.\n"
        "\n"
        "  - preview.css     - Includes rules for email-style headers.\n"
        "  - fountain.css    - Used by native fountain processor.\n"
        "  - markdown.css    - Used by the native markdown processor\n"
        "  - screenplain.css - Used by screenplain for fountain screenplays\n"
        "\n"
        "  - pandoc.css          - Used by Pandoc when a [format] file does "
        "not exist.\n"
        "  - pandoc-[format].css - Used by Pandoc for files written in "
        "[format]\n"
        "                          For example, pandoc-markdown.css\n"
        "\n"
        " The following option enables an extra, user-specified css file.  "
        "Included css files will be created when first used.\n"
        "  - extra-dark.css    - Dark theme.\n"
        "  - extra-invert.css  - Inverts everything.\n"
        "  - extra-media.css*  - @media rules to detect whether desktop uses "
        "dark theme.\n"
        "  - [filename]        - Create your own stylesheet\n"
        "  - disable           - Don't use any extra css files")};
  std::string extra_css{"extra-media.css"};

 private:
  static void document_signal(GObject *obj, GeanyDocument *doc,
                              PreviewWebView *self);

  static bool editor_notify(GObject *object, GeanyEditor *editor,
                            SCNotification *nt, PreviewWebView *self);
};
