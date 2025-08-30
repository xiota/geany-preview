// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "converter.h"

class ConverterCmarkGfm final : public Converter {
 public:
  ~ConverterCmarkGfm() override = default;

  std::string_view id() const override {
    return "cmark-gfm";
  }
  std::string toHtml(std::string_view source) override;
};
