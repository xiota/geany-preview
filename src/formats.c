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

GString *pandoc(const char *work_dir, const char *input, const char *type,
                const uint options) {
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

  if (!(options & PANDOC_FRAGMENT)) {
    g_ptr_array_add(args, g_strdup("--standalone"));
  }
  if (options & PANDOC_TOC) {
    g_ptr_array_add(args, g_strdup("--toc"));
  }

  g_ptr_array_add(args, NULL);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);

  if (!proc) {
    // command not found, FmtProcess will print warning
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
    // command not found, FmtProcess will print warning
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

  // use screenplain.css file from user config dir
  // if it does not exist, copy it from the system config dir
  char *css_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                  "preview", "screenplain.css", NULL);
  char *css_dn = g_path_get_dirname(css_fn);
  g_mkdir_with_parents(css_dn, 0755);

  if (!g_file_test(css_fn, G_FILE_TEST_EXISTS)) {
    if (g_file_test(PREVIEW_SCREENPLAIN_CSS, G_FILE_TEST_EXISTS)) {
      char *contents = NULL;
      size_t length = 0;
      if (g_file_get_contents(PREVIEW_SCREENPLAIN_CSS, &contents, &length,
                              NULL)) {
        g_file_set_contents(css_fn, contents, length, NULL);
        g_free(contents);
      }
    }
  }
  if (g_file_test(css_fn, G_FILE_TEST_EXISTS)) {
    g_ptr_array_add(args, g_strdup_printf("--css=%s", css_fn));
  }
  g_free(css_dn);
  g_free(css_fn);

  // end of args
  g_ptr_array_add(args, NULL);

  // rutn program
  FmtProcess *proc =
      fmt_process_open(work_dir, (const char *const *)args->pdata);

  if (!proc) {
    // command not found, FmtProcess will print warning
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
