/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>

#include <CLI/CLI.hpp>
#include <fstream>
#include <string>

#include "auxiliary.hxx"
#include "config.h"
#include "fountain.hxx"

int main(int argc, char ** argv) {
  auto cmd = std::string{argv[0]};
  cmd = cmd.substr(cmd.find_last_of("/\\") + 1);

  CLI::App app;

  // input file
  std::string input;
  app.add_option("-i, --input", input, "input file, default: stdin")
      ->option_text("<file>")
      ->default_val("/dev/stdin");

  // output file
  std::string output_file;
  app.add_option("-o, --output", output_file, "output file, default: stdout")
      ->option_text("<file>")
      ->default_val("/dev/stdout");

  // option output type
  std::string type;
  std::string css_fn;
  std::map<std::string_view, std::string_view> css_list {
#if ENABLE_EXPORT_PDF
    {"pdf", ""},
#endif
        {"html", "fountain-html.css"}, {"fdx", ""}, {"screenplain", "screenplain.css"},
        {"textplay", "textplay.css"}, {"xml", "fountain-xml.css"},
  };

  type = "xml";  // default
  for (const auto & [key, val] : css_list) {
    if (cmd.find(key) != std::string::npos) {
      type = key;
      css_fn = val;
      break;
    }
  }

  app.add_option("-t, --type", type, "output type")->option_text("<type>")->default_val(type);

  // list types
  bool list_types = false;
  app.add_flag("--list-types", list_types, "list available types and exit");

  // css protocol+path+file
  app.add_option("-c, --css-name", css_fn, "css protocol, path, and filename")
      ->option_text("<file>");

  // css protocol+path
  std::string css_path;
  app.add_option("-p, --css-path", css_path, "css protocol and path - filename varies by type")
      ->option_text("<path>")
      ->default_val(CSS_PATH);

  // css embed in output
  bool css_embed = false;
  app.add_flag("-e, --css-embed", css_embed, "embed css in output");

  // version
  app.set_version_flag(
      "-V, --version", cmd + " " VERSION, "Print version information and exit"
  );

  // help
  app.set_help_flag("-h, --help", "Print this help message and exit");

  // parse options
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError & e) {
    return app.exit(e);
  }

  // list output types and exit
  if (list_types) {
    std::cout << "output types:" << std::endl;
    for (const auto & [key, val] : css_list) {
      std::cout << "   " << key << std::endl;
    }
    return 0;
  }

  // read input file
  input = file_get_contents(input);

  // execute desired action
  std::string output;
#if ENABLE_EXPORT_PDF
  if (type == "pdf") {
    Fountain::ftn2pdf(output_file, input);
    return 0;
  } else
#endif
      if (type == "html") {
    output = Fountain::ftn2html(input, rtrim_inplace(css_path, "/") + "/" + css_fn, css_embed);
  } else if (type == "fdx") {
    output = Fountain::ftn2fdx(input);
  } else if (type == "screenplain") {
    output = Fountain::ftn2screenplain(
        input, rtrim_inplace(css_path, "/") + "/" + css_fn, css_embed
    );
  } else if (type == "textplay") {
    output =
        Fountain::ftn2textplay(input, rtrim_inplace(css_path, "/") + "/" + css_fn, css_embed);
  } else {
    // default: xml
    output = Fountain::ftn2xml(input, rtrim_inplace(css_path, "/") + "/" + css_fn, css_embed);
  }

  // write output
  file_set_contents(output_file, output);

  return 0;
}
