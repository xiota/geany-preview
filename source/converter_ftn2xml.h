// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "converter.h"

class ConverterFtn2xml final : public Converter {
 public:
  ~ConverterFtn2xml() override = default;

  std::string_view id() const override {
    return "ftn2xml";
  }

  std::string_view toHtml(std::string_view source) override;

 private:
  mutable std::string html_;
};
