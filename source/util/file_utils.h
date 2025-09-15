// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <gio/gio.h>
#include <sstream>
#include <stdexcept>
#include <string>

namespace FileUtils {

struct FileWatchHandle {
  GFile *file = nullptr;
  GFileMonitor *monitor = nullptr;
  guint timerId = 0;
  unsigned int delayMs = 150;
  std::function<void()> callback;
};

// Shared atomic guard for all watchers
inline std::atomic<bool> &reloadInProgress() {
  static std::atomic<bool> flag{ false };
  return flag;
}

// Timer callback — runs the user callback if not already in progress
static gboolean reloadCb(gpointer userData) {
  auto *handle = static_cast<FileWatchHandle *>(userData);

  bool expected = false;
  if (!reloadInProgress().compare_exchange_strong(expected, true)) {
    return G_SOURCE_REMOVE;  // already running
  }

  if (handle->callback) {
    handle->callback();
  }

  reloadInProgress().store(false);
  handle->timerId = 0;
  return G_SOURCE_REMOVE;
}

// GFileMonitor "changed" signal handler
static void
onChangedCb(GFileMonitor *, GFile *, GFile *, GFileMonitorEvent event, gpointer userData) {
  auto *handle = static_cast<FileWatchHandle *>(userData);

  if (event == G_FILE_MONITOR_EVENT_CHANGED || event == G_FILE_MONITOR_EVENT_CREATED ||
      event == G_FILE_MONITOR_EVENT_MOVED_IN) {
    if (handle->timerId != 0) {
      g_source_remove(handle->timerId);
    }
    handle->timerId = g_timeout_add(handle->delayMs, reloadCb, handle);
  }
}

// Create a file watch
inline void watchFile(
    FileWatchHandle &handle,
    const std::filesystem::path &path,
    std::function<void()> onChange,
    unsigned int delayMs = 150
) {
  handle.file = g_file_new_for_path(path.string().c_str());
  handle.monitor = g_file_monitor_file(handle.file, G_FILE_MONITOR_NONE, nullptr, nullptr);
  handle.delayMs = delayMs;
  handle.callback = std::move(onChange);

  if (handle.monitor) {
    g_signal_connect(handle.monitor, "changed", G_CALLBACK(onChangedCb), &handle);
  }
}

// Stop watching
inline void stopWatching(FileWatchHandle &handle) {
  if (handle.timerId) {
    g_source_remove(handle.timerId);
    handle.timerId = 0;
  }
  if (G_IS_OBJECT(handle.monitor)) {
    g_object_unref(handle.monitor);
    handle.monitor = nullptr;
  }
  if (G_IS_OBJECT(handle.file)) {
    g_object_unref(handle.file);
    handle.file = nullptr;
  }
}

// Read an entire file into a string
inline std::string readFileToString(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Check if a path exists and is a regular file
inline bool fileExists(const std::filesystem::path &path) noexcept {
  std::error_code ec;
  return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

}  // namespace FileUtils
