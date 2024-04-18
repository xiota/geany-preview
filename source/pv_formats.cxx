/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pv_formats.hxx"

#include "ftn2xml/auxiliary.hxx"
#include "config.h"
#include "process.hxx"

extern class PreviewSettings *gSettings;

std::string find_css(const std::string &css_fn) {
  std::string css_path =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", css_fn.c_str(), nullptr));

  std::string css_dn = cstr_assign(g_path_get_dirname(css_path.c_str()));
  g_mkdir_with_parents(css_dn.c_str(), 0755);

  if (g_file_test(css_path.c_str(), G_FILE_TEST_EXISTS)) {
    return css_path;
  } else {
    return {};
  }
}

std::string find_copy_css(const std::string &css_fn,
                          const std::string &src_fn) {
  std::string css_path =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", css_fn.c_str(), nullptr));

  std::string css_dn = cstr_assign(g_path_get_dirname(css_path.c_str()));
  g_mkdir_with_parents(css_dn.c_str(), 0755);

  if (!g_file_test(css_path.c_str(), G_FILE_TEST_EXISTS)) {
    if (g_file_test(src_fn.c_str(), G_FILE_TEST_EXISTS)) {
      std::string contents = file_get_contents(src_fn);
      if (!contents.empty()) {
        file_set_contents(css_path, contents);
      }
    }
  }
  if (g_file_test(css_path.c_str(), G_FILE_TEST_EXISTS)) {
    return css_path;
  } else {
    return {};
  }
}

std::string pandoc(const std::string &work_dir, const std::string &input,
                   const std::string &from_format) {
  if (input.empty()) {
    return {};
  }
  if (gSettings->pandoc_disabled) {
    return std::string("<pre>") + _("Pandoc has been disabled.") +
           std::string("</pre>");
  }

  std::vector<std::string> args_str;
  args_str.push_back("pandoc");

  if (!from_format.empty()) {
    args_str.push_back("--from=" + from_format);
  }
  args_str.push_back("--to=html");
  args_str.push_back("--quiet");

  if (!gSettings->pandoc_fragment) {
    args_str.push_back("--standalone");
  }
  if (gSettings->pandoc_toc) {
    args_str.push_back("--toc");
    args_str.push_back("--toc-depth=6");
  }

  // use pandoc-format.css file from user config dir
  // if it does not exist, copy it from the system config dir
  std::string css = "pandoc-" + from_format + ".css";
  std::string css_fn = find_css(css);
  if (css_fn.empty()) {
    css_fn = find_copy_css("pandoc.css", PREVIEW_CSS_PANDOC);
  }
  if (!css_fn.empty()) {
    args_str.push_back("--css=" + css_fn);
  }

  // run program
  FmtProcess *proc = fmt_process_open(work_dir, args_str);

  if (!proc) {
    // command not found, FmtProcess will print warning
    return {};
  }

  std::string strOutput;
  if (!fmt_process_run(proc, input, strOutput)) {
    g_warning(_("Failed to format document range"));
    fmt_process_close(proc);
    return {};
  }

  return strOutput;
}

std::string asciidoctor(const std::string &work_dir, const std::string &input) {
  if (input.empty()) {
    return {};
  }

  std::vector<std::string> args_str;
  args_str.push_back("asciidoctor");

  if (!work_dir.empty()) {
    args_str.push_back("--base-dir=" + work_dir);
  }
  args_str.push_back("--quiet");
  args_str.push_back("--out-file=-");
  args_str.push_back("-");

  // run program
  FmtProcess *proc = fmt_process_open(work_dir, args_str);

  if (!proc) {
    // command not found
    return {};
  }

  std::string strOutput;
  if (!fmt_process_run(proc, input, strOutput)) {
    g_warning(_("Failed to format document range"));
    fmt_process_close(proc);
    return {};
  }

  // attach asciidoctor.css if it exists
  std::string css_fn =
      find_copy_css("asciidoctor.css", PREVIEW_CSS_ASCIIDOCTOR);

  if (!css_fn.empty()) {
    std::string css_contents = file_get_contents(css_fn);

    std::size_t pos = strOutput.find("</head>");
    if (pos != std::string::npos) {
      std::string rep_text = "\n<style type='text/css'>\n" + css_contents +
                             "\n</style>\n</head>\n";
      strOutput.insert(pos, rep_text);
    } else {
      strOutput = "<!DOCTYPE html>\n<html>\n<head>\n<style type='text/css'>\n" +
                  css_contents + "\n</style>\n</head>\n<body>\n" + strOutput +
                  "</body></html>";
    }
  }
  return strOutput;
}

std::string asciidoc(const std::string &work_dir, const std::string &input) {
  if (input.empty()) {
    return {};
  }

  std::vector<std::string> args_str;
  args_str.push_back("asciidoc");

  args_str.push_back("--out-file=-");
  args_str.push_back("-");

  // run program
  FmtProcess *proc = fmt_process_open(work_dir, args_str);

  if (!proc) {
    // command not found
    return {};
  }

  std::string strOutput;
  if (!fmt_process_run(proc, input, strOutput)) {
    g_warning(_("Failed to format document range"));
    fmt_process_close(proc);
    return {};
  }

  // attach asciidoctor.css if it exists
  std::string css_fn =
      find_copy_css("asciidoctor.css", PREVIEW_CSS_ASCIIDOCTOR);

  if (!css_fn.empty()) {
    std::string css_contents = file_get_contents(css_fn);

    std::size_t pos = strOutput.find("</head>");
    if (pos != std::string::npos) {
      std::string rep_text = "\n<style type='text/css'>\n" + css_contents +
                             "\n</style>\n</head>\n";
      strOutput.insert(pos, rep_text);
    } else {
      strOutput = "<!DOCTYPE html>\n<html>\n<head>\n<style type='text/css'>\n" +
                  css_contents + "\n</style>\n</head>\n<body>\n" + strOutput +
                  "</body></html>";
    }
  }
  return strOutput;
}

// This function makes enabling cmark-gfm extensions easier
void addMarkdownExtension(cmark_parser *parser, const std::string &extName) {
  cmark_syntax_extension *ext = cmark_find_syntax_extension(extName.c_str());
  if (ext) {
    cmark_parser_attach_syntax_extension(parser, ext);
  }
}

std::string cmark_gfm(const std::string &input) {
  int options = CMARK_OPT_TABLE_PREFER_STYLE_ATTRIBUTES | CMARK_OPT_FOOTNOTES |
                CMARK_OPT_SMART;

  cmark_gfm_core_extensions_ensure_registered();

  cmark_parser *parser = cmark_parser_new(options);

  addMarkdownExtension(parser, "strikethrough");
  addMarkdownExtension(parser, "table");
  addMarkdownExtension(parser, "tasklist");

  cmark_node *doc;
  cmark_parser_feed(parser, input.c_str(), input.length());
  doc = cmark_parser_finish(parser);
  cmark_parser_free(parser);

  // Render
  std::string output = cstr_assign(cmark_render_html(doc, options, nullptr));
  cmark_node_free(doc);

  std::string css_fn = find_copy_css("markdown.css", PREVIEW_CSS_MARKDOWN);
  if (!css_fn.empty()) {
    std::string css_contents = file_get_contents(css_fn);
    output = "<html><head><style type='text/css'>\n" + css_contents +
             "</style>\n</head><body>" + output + "</body></html>";
  }

  return output;
}
