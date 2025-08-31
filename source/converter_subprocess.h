// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "converter.h"

class ConverterSubprocess : public Converter {
 public:
  virtual ~ConverterSubprocess() = default;

  std::string_view toHtml(std::string_view source) override;

 protected:
  explicit ConverterSubprocess(std::vector<std::string> base_args);

  // Subclasses can override to adjust args before running
  virtual std::vector<std::string> buildCommandArgs() const;

  std::vector<std::string> base_args_;
  std::string html_;
};
