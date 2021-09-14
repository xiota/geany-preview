/*
 * process.c
 *
 * Copyright 2013 Matthew <mbrush@codebrainz.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "process.h"

#define IO_BUF_SIZE 4096

struct FmtProcess
{
  GPid child_pid;
  GIOChannel *ch_in, *ch_out;
  int return_code;
  unsigned long exit_handler;
};

static void on_process_exited(GPid pid, int status, FmtProcess *proc)
{
  g_spawn_close_pid(pid);
  proc->child_pid = 0;

  // FIXME: is it automatically removed?
  if (proc->exit_handler > 0)
  {
    g_source_remove(proc->exit_handler);
    proc->exit_handler = 0;
  }

  if (proc->ch_in)
  {
    g_io_channel_shutdown(proc->ch_in, true, NULL);
    g_io_channel_unref(proc->ch_in);
    proc->ch_in = NULL;
  }

  if (proc->ch_out)
  {
    g_io_channel_shutdown(proc->ch_out, true, NULL);
    g_io_channel_unref(proc->ch_out);
    proc->ch_out = NULL;
  }

  proc->return_code = status;
}

FmtProcess *fmt_process_open(const char *work_dir, const char *const *argv)
{
  FmtProcess *proc;
  GError *error = NULL;
  int fd_in = -1, fd_out = -1;

  proc = g_new0(FmtProcess, 1);

  if (!g_spawn_async_with_pipes(work_dir, (char **)argv, NULL,
                                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                NULL, NULL, &proc->child_pid, &fd_in, &fd_out,
                                NULL, &error))
  {
    g_warning("Failed to create subprocess: %s", error->message);
    g_error_free(error);
    g_free(proc);
    return NULL;
  }

  proc->return_code = -1;
  proc->exit_handler = g_child_watch_add(
      proc->child_pid, (GChildWatchFunc)on_process_exited, proc);

  // TODO: handle windows
  proc->ch_in = g_io_channel_unix_new(fd_in);
  proc->ch_out = g_io_channel_unix_new(fd_out);

  return proc;
}

int fmt_process_close(FmtProcess *proc)
{
  int ret_code = proc->return_code;

  if (proc->ch_in)
  {
    g_io_channel_shutdown(proc->ch_in, true, NULL);
    g_io_channel_unref(proc->ch_in);
  }

  if (proc->ch_out)
  {
    g_io_channel_shutdown(proc->ch_out, true, NULL);
    g_io_channel_unref(proc->ch_out);
  }

  if (proc->child_pid > 0)
  {
    if (proc->exit_handler > 0)
      g_source_remove(proc->exit_handler);
    g_spawn_close_pid(proc->child_pid);
  }

  g_free(proc);

  return ret_code;
}

bool fmt_process_run(FmtProcess *proc, const char *str_in, size_t in_len,
                     GString *str_out)
{
  GIOStatus status;
  GError *error = NULL;
  bool read_complete = false;
  size_t in_off = 0;

  if (str_in && in_len)
  {

    do
    { // until all text is pushed into process's stdin

      size_t bytes_written = 0;
      size_t write_size_remaining = in_len - in_off;
      size_t size_to_write = MIN(write_size_remaining, IO_BUF_SIZE);

      // Write some data to process's stdin
      error = NULL;
      status = g_io_channel_write_chars(proc->ch_in, str_in + in_off,
                                        size_to_write, &bytes_written, &error);

      in_off += bytes_written;

      if (status == G_IO_STATUS_ERROR)
      {
        g_warning("Failed writing to subprocess's stdin: %s", error->message);
        g_error_free(error);
        return false;
      }

    } while (in_off < in_len);
  }

  // Flush it and close it down
  g_io_channel_shutdown(proc->ch_in, true, NULL);
  g_io_channel_unref(proc->ch_in);
  proc->ch_in = NULL;

  // All text should be written to process's stdin by now, read the
  // rest of the process's stdout.
  while (!read_complete)
  {
    char *tail_string = NULL;
    size_t tail_len = 0;

    error = NULL;
    status =
        g_io_channel_read_to_end(proc->ch_out, &tail_string, &tail_len, &error);

    if (tail_len > 0)
      g_string_append_len(str_out, tail_string, tail_len);

    g_free(tail_string);

    if (status == G_IO_STATUS_ERROR)
    {
      g_warning("Failed to read rest of subprocess's stdout: %s",
                error->message);
      g_error_free(error);
      return false;
    }
    else if (status == G_IO_STATUS_AGAIN)
    {
      continue;
    }
    else
    {
      read_complete = true;
    }
  }

  return true;
}
