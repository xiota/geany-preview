// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "converter.h"
#include <string>
#include <string_view>

class ConverterCmarkGfm final : public Converter {
 public:
  ~ConverterCmarkGfm() override = default;

  std::string_view id() const override {
    return "cmark-gfm";
  }
  std::string toHtml(std::string_view source) override;
};
