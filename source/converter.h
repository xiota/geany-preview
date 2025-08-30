// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

class Converter {
 public:
  virtual ~Converter() = default;

  virtual std::string_view id() const = 0;

  virtual std::string toHtml(std::string_view source) = 0;
};
