// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "converter_subprocess.h"

class ConverterAsciidoctor final : public ConverterSubprocess {
 public:
  ConverterAsciidoctor() : ConverterSubprocess({ "asciidoctor", "-o", "-", "-" }) {}

  std::string_view id() const noexcept override {
    return "asciidoctor";
  }
};
