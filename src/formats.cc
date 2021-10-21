/* -*- C++ -*-
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

#include <regex>

#include "auxiliary.h"
#include "formats.h"
#include "prefs.h"
#include "process.h"

std::string find_css(std::string const &css_fn) {
  std::string css_path =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", css_fn.c_str(), nullptr));

  // cache previous result to reduce filesystem access
  static std::string prev = css_path;
  if (prev == css_path) {
    return css_path;
  }

  std::string css_dn = cstr_assign(g_path_get_dirname(css_path.c_str()));
  g_mkdir_with_parents(css_dn.c_str(), 0755);

  if (g_file_test(css_path.c_str(), G_FILE_TEST_EXISTS)) {
    prev = css_path;
    return css_path;
  } else {
    prev.clear();
    return {};
  }
}

std::string find_copy_css(std::string const &css_fn,
                          std::string const &src_fn) {
  std::string css_path =
      cstr_assign(g_build_filename(geany_data->app->configdir, "plugins",
                                   "preview", css_fn.c_str(), nullptr));

  // cache previous result to reduce filesystem access
  static std::string prev = css_path;
  if (prev == css_path) {
    return css_path;
  }

  std::string css_dn = cstr_assign(g_path_get_dirname(css_path.c_str()));
  g_mkdir_with_parents(css_dn.c_str(), 0755);

  if (!g_file_test(css_path.c_str(), G_FILE_TEST_EXISTS)) {
    if (g_file_test(src_fn.c_str(), G_FILE_TEST_EXISTS)) {
      std::string contents = file_get_contents(src_fn);
      if (!contents.empty()) {
        file_set_contents(css_path, contents);
      }
    }
  }
  if (g_file_test(css_path.c_str(), G_FILE_TEST_EXISTS)) {
    prev = css_path;
    return css_path;
  } else {
    prev.clear();
    return {};
  }
}

char *pandoc(char const *work_dir, char const *input, char const *from_format) {
  if (input == nullptr) {
    return nullptr;
  }
  if (settings.pandoc_disabled) {
    return g_strjoin(nullptr, "<pre>", _("Pandoc has been disabled."), "</pre>",
                     nullptr);
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
  std::string css = cstr_assign(g_strdup_printf("pandoc-%s.css", from_format));
  std::string css_fn = find_css(css);
  if (css_fn.empty()) {
    css_fn = find_copy_css("pandoc.css", PREVIEW_CSS_PANDOC);
  }
  if (!css_fn.empty()) {
    g_ptr_array_add(args, g_strdup_printf("--css=%s", css_fn.c_str()));
  }

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

  return g_string_free(output, false);
}

char *asciidoctor(char const *work_dir, char const *input) {
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
  std::string css_fn =
      find_copy_css("asciidoctor.css", PREVIEW_CSS_ASCIIDOCTOR);
  if (!css_fn.empty()) {
    if (SUBSTR("</head>", output->str)) {
      try {
        std::string rep_text{
            "\n<link rel=\"stylesheet\" type=\"text/css\" href=\"file://"};
        rep_text += css_fn;
        rep_text += "\">\n</head>\n";

        static const std::regex re_head(R"(</head>)");
        std::string out_text =
            std::regex_replace(output->str, re_head, rep_text,
                               std::regex_constants::format_first_only);

        GSTRING_FREE(output);
        output = g_string_new(out_text.c_str());
      } catch (std::regex_error &e) {
        printf("regex error in asciidoctor\n");
      }
    } else {
      std::string out_text{
          "<html><head><link rel=\"stylesheet\" "
          "type=\"text/css\" href=\"file://"};
      out_text += css_fn;
      out_text += R"("></head><body>)";
      out_text += output->str;
      out_text += "</body></html>";

      GSTRING_FREE(output);
      output = g_string_new(out_text.c_str());
    }
  }
  return g_string_free(output, false);
}

char *screenplain(char const *work_dir, char const *input,
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
  std::string css = cstr_assign(g_strdup("screenplain.css"));
  std::string css_fn = find_copy_css(css, PREVIEW_CSS_SCREENPLAIN);
  if (!css_fn.empty()) {
    g_ptr_array_add(args, g_strdup_printf("--css=%s", css_fn.c_str()));
  }

  // end of args
  g_ptr_array_add(args, nullptr);

  // run program
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

  return g_string_free(output, false);
}
