// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_pandoc.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <glib.h>

#include "subprocess.h"

ConverterPandoc::ConverterPandoc(std::string_view from_format)
    : from_format_(from_format), base_args_{"pandoc", "-f", from_format_, "-t", "html", "-"} {}

std::string_view ConverterPandoc::toHtml(std::string_view source) {
  GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

  Subprocess runner;
  Subprocess::Result result;

  if (!runner.run(base_args_, source, [&](Subprocess::Result res) {
        result = std::move(res);
        g_main_loop_quit(loop);
      })) {
    g_main_loop_unref(loop);
    html_ =
        "<strong>Conversion failed</strong><br/>"
        "<pre style=\"white-space: pre-wrap;\">"
        "Failed to start pandoc process."
        "</pre>";
    return std::string_view(html_);
  }

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  if (result.exit_status == 0) {
    html_ = std::move(result.stdout_data);
  } else {
    html_ =
        "<strong>Conversion failed</strong><br/>"
        "<pre style=\"white-space: pre-wrap;\">"
        "pandoc exited with status " +
        std::to_string(result.exit_status);
    if (!result.stderr_data.empty()) {
      html_ += ": " + result.stderr_data;
    }
    html_ += "</pre>";
  }

  return std::string_view(html_);
}
