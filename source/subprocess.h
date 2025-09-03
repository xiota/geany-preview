// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
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

 private:
  static std::chrono::seconds nextCooldown(std::chrono::seconds current) noexcept;

  struct CacheEntry {
    bool found{ false };
    std::chrono::steady_clock::time_point last_check{};
    std::chrono::seconds cooldown{ std::chrono::seconds(1) };
  };

  static std::unordered_map<std::string, CacheEntry> binary_cache_;

  // Adaptive backoff parameters: 1s start, 1.25x growth, 5min cap.
  static constexpr auto kStartCooldown = std::chrono::seconds(1);
  static constexpr int kBackoffNum = 5;  // multiplier numerator (1.25x)
  static constexpr int kBackoffDen = 4;  // multiplier denominator
  static constexpr auto kMaxCooldown = std::chrono::seconds(300);  // 5 minutes
};
