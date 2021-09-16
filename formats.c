/*
 * Preview Geany Plugin
 * Copyright 2021 xiota
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

#include "formats.h"
#include "process.h"

GString *pandoc(const char *work_dir, const char *input, char *type) {
  if (input == NULL) {
    return NULL;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("pandoc"));

  if (type != NULL) {
    g_ptr_array_add(args, g_strdup_printf("--from=%s", type));
  }
  g_ptr_array_add(args, g_strdup("--to=html"));
  g_ptr_array_add(args, g_strdup("--quiet"));
  g_ptr_array_add(args, NULL);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);

  if (!proc) {
    // command not found
    g_warning("%s not found", (char *)args->pdata[0]);
    return NULL;
  }
  g_ptr_array_free(args, TRUE);

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning("Failed to format document range");
    g_string_free(output, TRUE);
    fmt_process_close(proc);
    return NULL;
  }

  return output;
}

GString *asciidoctor(const char *work_dir, const char *input) {
  if (input == NULL) {
    return NULL;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("asciidoctor"));

  if (work_dir != NULL) {
    g_ptr_array_add(args, g_strdup_printf("--base-dir=%s", work_dir));
  }
  g_ptr_array_add(args, g_strdup("--quiet"));
  g_ptr_array_add(args, g_strdup("--out-file=-"));
  g_ptr_array_add(args, g_strdup("-"));
  g_ptr_array_add(args, NULL);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);

  if (!proc) {
    // command not found
    g_warning("%s not found", (char *)args->pdata[0]);
    return NULL;
  }
  g_ptr_array_free(args, TRUE);

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning("Failed to format document range");
    g_string_free(output, TRUE);
    fmt_process_close(proc);
    return NULL;
  }

  return output;
}

GString *screenplain(const char *work_dir, const char *input,
                     const char *format) {
  if (input == NULL) {
    return NULL;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("screenplain"));

  if (format != NULL) {
    g_ptr_array_add(args, g_strdup_printf("--format=%s", format));
  } else {
    g_ptr_array_add(args, g_strdup("--format=html"));
  }
  g_ptr_array_add(args, NULL);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);

  if (!proc) {
    // command not found
    g_warning("%s not found", (char *)args->pdata[0]);
    return NULL;
  }
  g_ptr_array_free(args, TRUE);

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning("Failed to format document range");
    g_string_free(output, TRUE);
    fmt_process_close(proc);
    return NULL;
  }

  return output;
}
