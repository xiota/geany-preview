/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-FileCopyrightText: Copyright 2013 Matthew <mbrush@codebrainz.ca>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "process.hxx"

#define IO_BUF_SIZE 4096

struct FmtProcess {
  GPid child_pid;
  GIOChannel *ch_in, *ch_out;
  int return_code;
  unsigned long exit_handler;
};

static void on_process_exited(GPid pid, int status, FmtProcess * proc) {
  g_spawn_close_pid(pid);
  proc->child_pid = 0;

  // FIXME: is it automatically removed?
  if (proc->exit_handler > 0) {
    g_source_remove(proc->exit_handler);
    proc->exit_handler = 0;
  }

  if (proc->ch_in) {
    g_io_channel_shutdown(proc->ch_in, true, nullptr);
    g_io_channel_unref(proc->ch_in);
    proc->ch_in = nullptr;
  }

  if (proc->ch_out) {
    g_io_channel_shutdown(proc->ch_out, true, nullptr);
    g_io_channel_unref(proc->ch_out);
    proc->ch_out = nullptr;
  }

  proc->return_code = status;
}

FmtProcess * fmt_process_open(
    const std::string & work_dir, const std::vector<std::string> & argv_str
) {
  FmtProcess * proc;
  GError * error = nullptr;
  int fd_in = -1, fd_out = -1;

  proc = g_new0(FmtProcess, 1);

  std::vector<char *> argv;
  for (std::size_t i = 0; i < argv_str.size(); ++i) {
    argv.push_back(const_cast<char *>(argv_str[i].c_str()));
  }
  argv.push_back(nullptr);

  if (!g_spawn_async_with_pipes(
          work_dir.c_str(), (char **)argv.data(), nullptr,
          GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD), nullptr, nullptr,
          &proc->child_pid, &fd_in, &fd_out, nullptr, &error
      )) {
    g_warning(_("Failed to create subprocess: %s"), error->message);
    GERROR_FREE(error);
    GFREE(proc);
    return nullptr;
  }

  proc->return_code = -1;
  proc->exit_handler =
      g_child_watch_add(proc->child_pid, (GChildWatchFunc)on_process_exited, proc);

  // TODO: handle windows
  proc->ch_in = g_io_channel_unix_new(fd_in);
  proc->ch_out = g_io_channel_unix_new(fd_out);

  return proc;
}

int fmt_process_close(FmtProcess * proc) {
  int ret_code = proc->return_code;

  if (proc->ch_in) {
    g_io_channel_shutdown(proc->ch_in, true, nullptr);
    g_io_channel_unref(proc->ch_in);
  }

  if (proc->ch_out) {
    g_io_channel_shutdown(proc->ch_out, true, nullptr);
    g_io_channel_unref(proc->ch_out);
  }

  if (proc->child_pid > 0) {
    if (proc->exit_handler > 0) {
      g_source_remove(proc->exit_handler);
    }
    g_spawn_close_pid(proc->child_pid);
  }

  GFREE(proc);
  return ret_code;
}

bool fmt_process_run(FmtProcess * proc, const std::string & str_in, std::string & str_out) {
  GIOStatus status;
  GError * error = nullptr;
  bool read_complete = false;
  std::size_t in_off = 0;

  if (!str_in.empty()) {
    do {
      // until all text is pushed into process's stdin
      gsize bytes_written = 0;
      int write_size_remaining = str_in.size() - in_off;
      int size_to_write = MIN(write_size_remaining, IO_BUF_SIZE);

      // Write some data to process's stdin
      error = nullptr;
      status = g_io_channel_write_chars(
          proc->ch_in, str_in.c_str() + in_off, size_to_write, &bytes_written, &error
      );

      in_off += bytes_written;

      if (status == G_IO_STATUS_ERROR) {
        g_warning(_("Failed writing to subprocess's stdin: %s"), error->message);
        GERROR_FREE(error);
        return false;
      }

    } while (in_off < str_in.size());
  }

  // Flush it and close it down
  g_io_channel_shutdown(proc->ch_in, true, nullptr);
  g_io_channel_unref(proc->ch_in);
  proc->ch_in = nullptr;

  // All text should be written to process's stdin by now, read the
  // rest of the process's stdout.
  while (!read_complete) {
    char * tail_string = nullptr;
    gsize tail_len = 0;

    error = nullptr;
    status = g_io_channel_read_to_end(proc->ch_out, &tail_string, &tail_len, &error);

    if (tail_len > 0) {
      str_out.append(tail_string);
    }

    GFREE(tail_string);

    if (status == G_IO_STATUS_ERROR) {
      g_warning(_("Failed to read rest of subprocess's stdout: %s"), error->message);
      GERROR_FREE(error);
      return false;
    } else if (status == G_IO_STATUS_AGAIN) {
      continue;
    } else {
      read_complete = true;
    }
  }

  return true;
}
