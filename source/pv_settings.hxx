/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>
#include <vector>

#include "pv_main.hxx"

#define PREVIEW_KF_GROUP "preview"

union TkuiSetting {
  bool tkuiBoolean;
  int tkuiInteger;
  double tkuiDouble;
  std::string tkuiString;
};

enum TkuiSettingType {
  TKUI_PREF_TYPE_NONE,
  TKUI_PREF_TYPE_BOOLEAN,
  TKUI_PREF_TYPE_INTEGER,
  TKUI_PREF_TYPE_DOUBLE,
  TKUI_PREF_TYPE_STRING,
};

class TkuiSettingPref {
 public:
  TkuiSettingType type;
  std::string name;
  std::string comment;
  bool session;
  TkuiSetting * setting;
};

class PreviewSettings {
 public:
  void initialize();

  void load();
  void save(bool bSession = false);
  void save_session();
  void reset();
  void kf_open();
  void kf_close();

 public:
  std::string config_file;

  std::string desc_startup_timeout =
      _("Set the duration (ms) to wait after startup before forcing webview "
        "update.");
  int startup_timeout = 350;

  std::string desc_update_interval_slow =
      _("Set the minimum number of milliseconds between calls for slow "
        "external programs.  This gives them time to finish "
        "processing.  Use a larger number for slower machines.");
  int update_interval_slow = 200;

  std::string desc_size_factor_slow =
      _("For large files, a longer delay is needed.  The delay in milliseconds "
        "is the filesize in bytes multiplied by size_factor_slow.");
  double size_factor_slow = 0.004;

  std::string desc_update_interval_fast =
      _("Even fast programs may need a delay to finish processing on slow "
        "computers.");
  int update_interval_fast = 25;
  double size_factor_fast = 0.001;

  std::string desc_default_type =
      _("The default type for new documents.  Use the format name or extension.\n"
        "  - none*\n"
        "  - markdown\n"
        "  - latex\n"
        "  - rst\n"
        "  - t2t\n"
        "  - ... etc");
  std::string default_type{"none"};

  std::string desc_processor_html =
      _("Control how HTML documents are processed.\n"
        "  - native* - Send the document directly to the webview.\n"
        "  - pandoc  - Use Pandoc to clean up the document and apply a "
        "stylesheet.\n"
        "  - disable - Turn off HTML previews.");
  std::string processor_html{"native"};

  std::string desc_processor_markdown =
      _("Set the Markdown processor.\n"
        "  - native* - Use libcmark-gfm (faster)\n"
        "  - pandoc  - Use Pandoc GFM   (applies additional pandoc options)\n"
        "  - disable - Turn off Markdown previews.");
  std::string processor_markdown{"native"};

  std::string desc_processor_asciidoc =
      _("Enables or disable AsciiDoc previews.\n"
        "  - asciidoctor* - Use asciidoctor.\n"
        "  - asciidoc     - Use asciidoc.\n"
        "  - disable      - Turn off AsciiDoc previews.");
  std::string processor_asciidoc{"asciidoctor"};

  std::string desc_processor_fountain =
      _("Set the Fountain processor.\n"
        "  - native*      - Use the built-in fountain processor\n"
        "  - disable      - Turn off Fountain screenplay previews");
  std::string processor_fountain{"native"};

  std::string desc_wiki_default =
      _("Set the format that Pandoc uses to interpret files with the "
        ".wiki extension.  It can also turn off wiki previews.  "
        "Options may be any format from `pandoc --list-input-formats`: "
        "disable, dokuwiki, *mediawiki*, tikiwiki, twiki, vimwiki");
  std::string wiki_default{"mediawiki"};

  std::string desc_pandoc_disabled = _("Disable Pandoc processing.");
  bool pandoc_disabled = false;

  std::string desc_pandoc_fragment =
      _("Control whether Pandoc produces standalone HTML files.  If "
        "set to true, stylesheets are effectively disabled.");
  bool pandoc_fragment = false;

  std::string desc_pandoc_toc =
      _("Control whether Pandoc inserts an auto-generated table of "
        "contents at the beginning of HTML files.  This might help "
        "with navigating long documents.");
  bool pandoc_toc = false;

  std::string desc_pandoc_markdown =
      _("How Pandoc interprets Markdown.  It has *no* effect unless Pandoc is "
        "set as the processor_markdown.  Options may be any format from "
        "`pandoc --list-input-formats`: commonmark, *markdown*, "
        "markdown_github, markdown_mmd, markdown_phpextra, markdown_strict");
  std::string pandoc_markdown{"markdown"};

  std::string desc_default_font_family =
      _("Set the font family used by the webview when another font is "
        "not specified by a stylesheet.");
  std::string default_font_family{"serif"};

  std::string desc_extended_types =
      _("Control whether the file extension is used to detect file "
        "types that are unknown to Geany.  This feature is needed to "
        "preview fountain, textile, wiki, muse, and org documents.");
  bool extended_types = true;

  std::string desc_snippet_window =
      _("Large documents are both slow to process and difficult to "
        "navigate.  The snippet settings allow a small region of "
        "interest to be previewed.  The trigger is the file size "
        "(bytes) that activates snippets.  The window is the size "
        "(bytes) around the caret that is processed.");
  int snippet_window = 5000;
  int snippet_trigger = 100000;

  std::string desc_snippet_html =
      _("Some document types don't work well with the snippets feature.  The "
        "following options disable (false) and enable (true) them.");
  bool snippet_html = false;
  bool snippet_markdown = true;
  bool snippet_asciidoctor = true;
  bool snippet_pandoc = true;
  bool snippet_fountain = true;

  std::string desc_extra_css =
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
        "  - disable           - Don't use any extra css files");
  std::string extra_css{"extra-media.css"};

 private:
  bool kf_has_key(const std::string & key);

  void add_setting(
      TkuiSetting * setting, const TkuiSettingType & type, const std::string & name,
      const std::string & comment, const bool & session
  );

 private:
  GKeyFile * keyfile = nullptr;
  std::vector<TkuiSettingPref> pref_entries;
};
