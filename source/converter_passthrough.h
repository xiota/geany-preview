// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "converter.h"

class ConverterPassthrough : public Converter {
 public:
  std::string_view id() const override {
    return "passthrough";
  }

  std::string toHtml(std::string_view source) override {
    return std::string{source};
  }
};
