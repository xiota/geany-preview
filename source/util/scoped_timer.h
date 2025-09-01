// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file scoped_timer.h
 * @brief Lightweight RAII timer for quick profiling of functions or code blocks.
 *
 * Starts timing on construction and logs elapsed time on destruction.
 * Accepts string literals (zero‑copy) or std::string/std::string_view (safe copy).
 *
 * Usage examples:
 *   PROFILE_CALL("expensiveOperation", expensiveOperation());
 *   PROFILE_CALL("expensiveOperation", expensiveOperation(), std::cout);
 *   {
 *       PROFILE_SCOPE("Full update sequence");
 *       PROFILE_SCOPE("Full update sequence", logFile);
 *       loadData();
 *       processData();
 *   } // Logs time here
 *
 * Output format:
 *   [Telemetry] <label> took <milliseconds> ms
 *
 * Output stream:
 *   By default, writes to std::cerr. Can be redirected to any std::ostream.
 *
 * Intended for development/debug builds to gather quick telemetry
 * without setting up a full profiler. Thread‑safe per instance.
 *
 * See scoped_timer.md for full usage notes.
 */

#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

class ScopedTimer {
 public:
  explicit ScopedTimer(const char *label, std::ostream &os = std::cerr)
      : label_view_(label), out_(os), start_(std::chrono::steady_clock::now()) {}

  explicit ScopedTimer(std::string_view label, std::ostream &os = std::cerr)
      : owned_(label),
        label_view_(owned_),
        out_(os),
        start_(std::chrono::steady_clock::now()) {}

  ~ScopedTimer() {
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count() / 1000.0;
    out_ << "[Telemetry] " << label_view_ << " took " << elapsed_ms << " ms" << std::endl;
  }

 private:
  std::string owned_;
  std::string_view label_view_;
  std::ostream &out_;
  std::chrono::steady_clock::time_point start_;
};

// Macros now accept optional ostream argument
#define PROFILE_CALL(label, expr, ...)         \
  do {                                         \
    ScopedTimer timer__(label, ##__VA_ARGS__); \
    expr;                                      \
  } while (0)

#define PROFILE_SCOPE(label, ...) ScopedTimer timer__(label, ##__VA_ARGS__)
