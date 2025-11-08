// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "converter.h"

class ConverterMd4c final : public Converter {
 public:
  ~ConverterMd4c() override = default;

  std::string_view id() const override {
    return "md4c";
  }
  std::string_view toHtml(std::string_view source) override;

 private:
  // Keeps the buffer alive until the next toHtml() call
  mutable std::unique_ptr<std::string> html_owner_;
  mutable std::string_view html_view_;
};
