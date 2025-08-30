// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "converter.h"

class ConverterPandoc final : public Converter {
 public:
  explicit ConverterPandoc(std::string_view from_format);

  std::string_view id() const noexcept {
    return from_format_;
  }

  std::string_view toHtml(std::string_view source) override;

 private:
  std::string from_format_;
  std::vector<std::string> base_args_;

  std::string html_;
};
