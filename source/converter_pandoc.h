// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "converter_subprocess.h"

class ConverterPandoc final : public ConverterSubprocess {
 public:
  explicit ConverterPandoc(std::string_view from_format)
      : ConverterSubprocess({"pandoc", "-f", std::string(from_format), "-t", "html", "-"}),
        from_format_(from_format) {}

  std::string_view id() const noexcept override {
    return from_format_;
  }

 private:
  std::string from_format_;
};
