// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_subprocess.h"

#include <string>
#include <string_view>
#include <utility>

#include <glib.h>

#include "subprocess.h"

ConverterSubprocess::ConverterSubprocess(std::vector<std::string> base_args)
    : base_args_(std::move(base_args)) {}

std::vector<std::string> ConverterSubprocess::buildCommandArgs() const {
  return base_args_;
}

std::string_view ConverterSubprocess::toHtml(std::string_view source) {
  GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

  Subprocess runner;
  Subprocess::Result result;

  if (!runner.run(buildCommandArgs(), source, [&](Subprocess::Result res) {
        result = std::move(res);
        g_main_loop_quit(loop);
      })) {
    g_main_loop_unref(loop);
    html_ =
        "<strong>Conversion failed</strong><br/>"
        "<pre style=\"white-space: pre-wrap;\">"
        "Failed to start process."
        "</pre>";
    return html_;
  }

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  if (result.exit_status == 0) {
    html_ = std::move(result.stdout_data);
  } else {
    html_ =
        "<strong>Conversion failed</strong><br/>"
        "<pre style=\"white-space: pre-wrap;\">"
        "Process exited with status " +
        std::to_string(result.exit_status);
    if (!result.stderr_data.empty()) {
      html_ += ": " + result.stderr_data;
    }
    html_ += "</pre>";
  }

  return html_;
}
