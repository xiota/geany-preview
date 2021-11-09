/*
 * Preview Geany Plugin
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

#include <regex>

#include "auxiliary.h"
#include "process.h"
#include "pv_formats.h"

std::string find_css(std::string const &css_fn) {
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

std::string find_copy_css(std::string const &css_fn,
                          std::string const &src_fn) {
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

std::string pandoc(std::string const &work_dir, std::string const &input,
                   std::string const &from_format) {
  if (input.empty()) {
    return {};
  }
  if (gSettings.pandoc_disabled) {
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

  if (!gSettings.pandoc_fragment) {
    args_str.push_back("--standalone");
  }
  if (gSettings.pandoc_toc) {
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

std::string asciidoctor(std::string const &work_dir, std::string const &input) {
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
    size_t pos = strOutput.find("</head>");
    if (pos != std::string::npos) {
      std::string rep_text{
          "\n<link rel=\"stylesheet\" "
          "type=\"text/css\" href=\"file://"};
      rep_text += css_fn + std::string("\">\n</head>\n");

      strOutput.insert(pos, rep_text);
    }
  } else {
    strOutput =
        std::string(
            "<!DOCTYPE html>\n<html>\n<head>\n"
            "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://") +
        css_fn + "\">\n</head>\n<body>\n" + strOutput + "</body></html>";
  }
  return strOutput;
}

std::string screenplain(std::string const &work_dir, std::string const &input,
                        std::string const &to_format) {
  if (input.empty()) {
    return {};
  }

  std::vector<std::string> args_str;
  args_str.push_back("screenplain");

  if (!to_format.empty()) {
    args_str.push_back("--format=" + to_format);
  } else {
    args_str.push_back("--format=html");
  }

  // use screenplain.css file from user config dir
  // if it does not exist, copy it from the system config dir
  std::string css = cstr_assign(g_strdup("screenplain.css"));
  std::string css_fn = find_copy_css(css, PREVIEW_CSS_SCREENPLAIN);
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
    return nullptr;
  }

  return strOutput;
}

// This function makes enabling cmark-gfm extensions easier
static void addMarkdownExtension(cmark_parser *parser,
                                 std::string const &extName) {
  cmark_syntax_extension *ext = cmark_find_syntax_extension(extName.c_str());
  if (ext) {
    cmark_parser_attach_syntax_extension(parser, ext);
  }
}

std::string cmark_gfm(std::string const &input) {
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
    output =
        "<html><head><link rel='stylesheet' "
        "type='text/css' href='file://" +
        css_fn + "'></head><body>" + output + "</body></html>";
  }

  return output;
}
