// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "subprocess.h"

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <glib.h>

std::unordered_map<std::string, Subprocess::CacheEntry> Subprocess::binary_cache_;

namespace {
constexpr std::size_t kIoBufSize = 4096;
constexpr int kNumStreams = 2;

struct AsyncContext {
  GPid pid{};
  GIOChannel *out_ch{nullptr};
  GIOChannel *err_ch{nullptr};
  std::ostringstream outbuf;
  std::ostringstream errbuf;
  Subprocess::CompletionHandler handler;
  int exit_status{-1};
  int streams_remaining{kNumStreams};  // stdout + stderr still open
  guint out_watch_id{0};
  guint err_watch_id{0};
  guint child_watch_id{0};
  bool cancelled{false};
};
static std::set<AsyncContext *> active_contexts;

bool writeAll(int fd, std::string_view data) noexcept {
  std::size_t offset = 0;
  while (offset < data.size()) {
    ssize_t written =
        ::write(fd, data.data() + offset, std::min(kIoBufSize, data.size() - offset));

    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      ::close(fd);
      return false;
    }
    if (written == 0) {
      ::close(fd);
      return false;
    }
    offset += static_cast<std::size_t>(written);
  }
  ::close(fd);  // signal EOF
  return true;
}

static void cleanupProcess(AsyncContext *ctx) {
  if (ctx->streams_remaining == 0 && ctx->exit_status != -1) {
    Subprocess::Result res{ctx->outbuf.str(), ctx->errbuf.str(), ctx->exit_status};
    ctx->handler(res);
    if (ctx->out_ch) {
      g_io_channel_unref(ctx->out_ch);
    }
    if (ctx->err_ch) {
      g_io_channel_unref(ctx->err_ch);
    }
    active_contexts.erase(ctx);
    delete ctx;
  }
}

gboolean readChannel(GIOChannel *source, GIOCondition condition, gpointer user_data) noexcept {
  auto *ctx = static_cast<AsyncContext *>(user_data);
  if (ctx->cancelled) {
    return FALSE;  // stop if cancelled
  }

  if (condition & (G_IO_HUP | G_IO_ERR)) {
    --ctx->streams_remaining;
    cleanupProcess(ctx);
    return FALSE;  // stop watching this channel
  }

  char buf[kIoBufSize];
  gsize bytes_read = 0;
  GError *err = nullptr;
  GIOStatus status = g_io_channel_read_chars(source, buf, sizeof buf, &bytes_read, &err);

  if (status == G_IO_STATUS_ERROR) {
    if (err) {
      ctx->errbuf << "[I/O error] " << err->message << "\n";
      g_error_free(err);
    }
    return TRUE;
  }
  if (bytes_read > 0) {
    if (source == ctx->out_ch) {
      ctx->outbuf.write(buf, bytes_read);
    } else {
      ctx->errbuf.write(buf, bytes_read);
    }
  }
  return TRUE;
}

void onChildExit(GPid pid, gint status, gpointer user_data) noexcept {
  auto *ctx = static_cast<AsyncContext *>(user_data);
  if (ctx->cancelled) {
    g_spawn_close_pid(pid);
    active_contexts.erase(ctx);
    delete ctx;
    return;
  }
  int code = -1;
  if (WIFEXITED(status)) {
    code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    code = 128 + WTERMSIG(status);
  }
  ctx->exit_status = code;
  g_spawn_close_pid(pid);

  cleanupProcess(ctx);
}
}  // namespace

std::chrono::seconds Subprocess::nextCooldown(std::chrono::seconds current) noexcept {
  if (current < kMaxCooldown) {
    long long cur = current.count();
    long long next = (cur * kBackoffNum + (kBackoffDen - 1)) / kBackoffDen;
    if (next <= cur) {
      next = cur + 1;
    }
    if (next > kMaxCooldown.count()) {
      next = kMaxCooldown.count();
    }
    return std::chrono::seconds(next);
  }
  return current;  // already at cap
}

bool Subprocess::run(
    const std::vector<std::string> &args,
    std::string_view input,
    CompletionHandler handler
) const {
  if (args.empty()) {
    return false;
  }

  const std::string &binary = args[0];
  auto now = std::chrono::steady_clock::now();
  auto it = binary_cache_.find(binary);
  if (it != binary_cache_.end()) {
    CacheEntry &entry = it->second;
    if (!entry.found) {
      // If still in cooldown, skip.
      if (entry.last_check.time_since_epoch().count() != 0 &&
          (now - entry.last_check) < entry.cooldown) {
        return false;
      }
      // Due for availability check.
      char *found_path = g_find_program_in_path(binary.c_str());
      bool found = (found_path != nullptr);
      if (found_path) {
        g_free(found_path);
      }
      entry.last_check = now;
      if (found) {
        // Reset on success.
        entry.found = true;
        entry.cooldown = kStartCooldown;
      } else {
        // Increase cooldown (cap and hold).
        entry.cooldown = nextCooldown(entry.cooldown);
        return false;
      }
    }
  }

  // Build argv for GLib
  std::vector<gchar *> argv;
  argv.reserve(args.size() + 1);
  for (auto &a : args) {
    argv.push_back(const_cast<gchar *>(a.c_str()));
  }
  argv.push_back(nullptr);

  GPid pid = 0;
  gint stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
  GError *error = nullptr;

  if (!g_spawn_async_with_pipes(
          nullptr,
          argv.data(),
          nullptr,
          static_cast<GSpawnFlags>(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
          nullptr,
          nullptr,
          &pid,
          &stdin_fd,
          &stdout_fd,
          &stderr_fd,
          &error
      )) {
    std::string msg = error ? error->message : "Unknown spawn error";
    if (error) {
      g_error_free(error);
    }
    // Mark as missing and apply backoff if spawn fails.
    auto &entry = binary_cache_[binary];
    if (entry.last_check.time_since_epoch().count() == 0) {
      entry.cooldown = kStartCooldown;
    }
    entry.found = false;
    entry.last_check = now;
    entry.cooldown = nextCooldown(entry.cooldown);
    Subprocess::Result res;
    res.stderr_data = "spawn failed: " + msg;
    res.exit_status = 127;
    handler(res);
    return false;
  }

  // Feed stdin
  if (!writeAll(stdin_fd, input)) {
    Subprocess::Result res;
    res.stderr_data = "stdin write failed";
    res.exit_status = 1;
    handler(res);
    return false;
  }

  // Prepare async context
  auto *ctx = new AsyncContext;
  ctx->pid = pid;
  ctx->handler = std::move(handler);
  ctx->out_ch = g_io_channel_unix_new(stdout_fd);
  ctx->err_ch = g_io_channel_unix_new(stderr_fd);
  g_io_channel_set_encoding(ctx->out_ch, nullptr, nullptr);
  g_io_channel_set_encoding(ctx->err_ch, nullptr, nullptr);
  g_io_channel_set_buffered(ctx->out_ch, FALSE);
  g_io_channel_set_buffered(ctx->err_ch, FALSE);
  g_io_channel_set_close_on_unref(ctx->out_ch, TRUE);
  g_io_channel_set_close_on_unref(ctx->err_ch, TRUE);

  ctx->out_watch_id = g_io_add_watch(
      ctx->out_ch, GIOCondition(G_IO_IN | G_IO_HUP | G_IO_ERR), readChannel, ctx
  );
  ctx->err_watch_id = g_io_add_watch(
      ctx->err_ch, GIOCondition(G_IO_IN | G_IO_HUP | G_IO_ERR), readChannel, ctx
  );
  ctx->child_watch_id = g_child_watch_add(pid, onChildExit, ctx);

  active_contexts.insert(ctx);
  return true;
}

// Cancel all active jobs, e.g. from plugin cleanup
void Subprocess::cancelAll() noexcept {
  for (auto *ctx : active_contexts) {
    ctx->cancelled = true;
    if (ctx->out_watch_id) {
      g_source_remove(ctx->out_watch_id);
      ctx->out_watch_id = 0;
    }
    if (ctx->err_watch_id) {
      g_source_remove(ctx->err_watch_id);
      ctx->err_watch_id = 0;
    }
    if (ctx->child_watch_id) {
      g_source_remove(ctx->child_watch_id);
      ctx->child_watch_id = 0;
    }
    if (ctx->out_ch) {
      g_io_channel_unref(ctx->out_ch);
    }
    if (ctx->err_ch) {
      g_io_channel_unref(ctx->err_ch);
    }
    // child pid will be closed in onChildExit or here:
    if (ctx->pid) {
      g_spawn_close_pid(ctx->pid);
    }
    delete ctx;
  }
  active_contexts.clear();
}
