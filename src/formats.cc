/*
 * Preview Geany Plugin
 * Copyright 2021 xiota
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "formats.h"

#include "prefs.h"
#include "process.h"

char *find_css(char const *css) {
  char *css_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                  "preview", css, nullptr);
  char *css_dn = g_path_get_dirname(css_fn);
  g_mkdir_with_parents(css_dn, 0755);
  GFREE(css_dn);

  if (g_file_test(css_fn, G_FILE_TEST_EXISTS)) {
    return css_fn;
  } else {
    return nullptr;
  }
}

char *find_copy_css(char const *css, char const *src) {
  char *css_fn = g_build_filename(geany_data->app->configdir, "plugins",
                                  "preview", css, nullptr);
  char *css_dn = g_path_get_dirname(css_fn);
  g_mkdir_with_parents(css_dn, 0755);
  GFREE(css_dn);

  if (!g_file_test(css_fn, G_FILE_TEST_EXISTS)) {
    if (g_file_test(src, G_FILE_TEST_EXISTS)) {
      char *contents = nullptr;
      size_t length = 0;
      if (g_file_get_contents(src, &contents, &length, nullptr)) {
        g_file_set_contents(css_fn, contents, length, nullptr);
        GFREE(contents);
      }
    }
  }
  if (g_file_test(css_fn, G_FILE_TEST_EXISTS)) {
    return css_fn;
  } else {
    return nullptr;
  }
}

GString *pandoc(char const *work_dir, char const *input,
                char const *from_format) {
  if (input == nullptr) {
    return nullptr;
  }
  if (settings.pandoc_disabled) {
    GString *output =
        g_string_new("<pre>" _("Pandoc has been disabled.") "</pre>");
    return output;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("pandoc"));

  if (from_format != nullptr) {
    g_ptr_array_add(args, g_strdup_printf("--from=%s", from_format));
  }
  g_ptr_array_add(args, g_strdup("--to=html"));
  g_ptr_array_add(args, g_strdup("--quiet"));

  if (!settings.pandoc_fragment) {
    g_ptr_array_add(args, g_strdup("--standalone"));
  }
  if (settings.pandoc_toc) {
    g_ptr_array_add(args, g_strdup("--toc"));
    g_ptr_array_add(args, g_strdup("--toc-depth=6"));
  }

  // use pandoc-format.css file from user config dir
  // if it does not exist, copy it from the system config dir
  char *css = g_strdup_printf("pandoc-%s.css", from_format);
  char *css_fn = find_css(css);
  if (!css_fn) {
    css_fn = find_copy_css("pandoc.css", PREVIEW_CSS_PANDOC);
  }
  if (css_fn) {
    g_ptr_array_add(args, g_strdup_printf("--css=%s", css_fn));
  }
  GFREE(css_fn);
  GFREE(css);

  // end of args
  g_ptr_array_add(args, nullptr);

  FmtProcess *proc =
      fmt_process_open(work_dir, (char const *const *)args->pdata);

  if (!proc) {
    // command not found, FmtProcess will print warning
    return nullptr;
  }
  g_ptr_array_free(args, true);

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning(_("Failed to format document range"));
    GSTRING_FREE(output);
    fmt_process_close(proc);
    return nullptr;
  }

  return output;
}

GString *asciidoctor(char const *work_dir, char const *input) {
  if (input == nullptr) {
    return nullptr;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("asciidoctor"));

  if (work_dir != nullptr) {
    g_ptr_array_add(args, g_strdup_printf("--base-dir=%s", work_dir));
  }
  g_ptr_array_add(args, g_strdup("--quiet"));
  g_ptr_array_add(args, g_strdup("--out-file=-"));
  g_ptr_array_add(args, g_strdup("-"));
  g_ptr_array_add(args, nullptr);  // end of args

  FmtProcess *proc =
      fmt_process_open(work_dir, (char const *const *)args->pdata);

  if (!proc) {
    // command not found, FmtProcess will print warning
    return nullptr;
  }
  g_ptr_array_free(args, true);

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning(_("Failed to format document range"));
    GSTRING_FREE(output);
    fmt_process_close(proc);
    return nullptr;
  }

  // attach asciidoctor.css if it exists
  char *css_fn = find_copy_css("asciidoctor.css", PREVIEW_CSS_ASCIIDOCTOR);
  if (css_fn) {
    if (REGEX_CHK("</head>", output->str)) {
      char *plain = g_strdup_printf(
          "\n<link rel='stylesheet' type='text/css' "
          "href='file://%s'>\n</head>\n",
          css_fn);
      g_string_replace(output, "</head>", plain, 1);
      GFREE(plain);
    } else {
      char *html = g_string_free(output, false);
      char *plain = g_strjoin(
          nullptr,
          "<html><head><link rel='stylesheet' type='text/css' href='file://",
          css_fn, "'></head><body>", html, "</body></html>", nullptr);
      output = g_string_new(plain);
      GFREE(html);
      GFREE(plain);
    }
  }

  return output;
}

GString *screenplain(char const *work_dir, char const *input,
                     char const *to_format) {
  if (input == nullptr) {
    return nullptr;
  }

  GPtrArray *args = g_ptr_array_new_with_free_func(g_free);
  g_ptr_array_add(args, g_strdup("screenplain"));

  if (to_format != nullptr) {
    g_ptr_array_add(args, g_strdup_printf("--format=%s", to_format));
  } else {
    g_ptr_array_add(args, g_strdup("--format=html"));
  }

  // use screenplain.css file from user config dir
  // if it does not exist, copy it from the system config dir
  char *css = g_strdup("screenplain.css");
  char *css_fn = find_copy_css(css, PREVIEW_CSS_SCREENPLAIN);
  if (css_fn) {
    g_ptr_array_add(args, g_strdup_printf("--css=%s", css_fn));
  }
  GFREE(css_fn);
  GFREE(css);

  // end of args
  g_ptr_array_add(args, nullptr);

  // rutn program
  FmtProcess *proc =
      fmt_process_open(work_dir, (char const *const *)args->pdata);

  if (!proc) {
    // command not found, FmtProcess will print warning
    return nullptr;
  }
  g_ptr_array_free(args, true);

  GString *output = g_string_sized_new(strlen(input));
  if (!fmt_process_run(proc, input, strlen(input), output)) {
    g_warning(_("Failed to format document range"));
    GSTRING_FREE(output);
    fmt_process_close(proc);
    return nullptr;
  }

  return output;
}
