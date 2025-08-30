// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class Subprocess {
 public:
  struct Result {
    std::string stdout_data;
    std::string stderr_data;
    int exit_status = -1;
  };

  using CompletionHandler = std::function<void(const Result &)>;

  Subprocess() noexcept = default;
  ~Subprocess() noexcept = default;

  // Non-copyable, non-movable if you want to manage lifecycle tightly
  Subprocess(const Subprocess &) noexcept = delete;
  Subprocess &operator=(const Subprocess &) noexcept = delete;

  // Returns true if the process was launched successfully, false otherwise.
  bool run(
      const std::vector<std::string> &args,
      std::string_view input,
      CompletionHandler handler
  ) const;

  static void cancelAll() noexcept;
};
