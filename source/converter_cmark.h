// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "converter.h"

class ConverterCmark final : public Converter {
 public:
  ~ConverterCmark() override = default;

  std::string_view id() const override {
    return "cmark";
  }
  std::string_view toHtml(std::string_view source) override;

 private:
  // Keeps the buffer alive until the next toHtml() call
  mutable std::unique_ptr<char, decltype(&free)> html_owner_{ nullptr, &free };
  mutable std::string_view html_view_;
};
