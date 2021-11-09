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

#pragma once

#include "tkui_addons.h"

#define PREVIEW_KF_GROUP "preview"

class PreviewSettings {
 public:
  PreviewSettings() = default;
  ~PreviewSettings();

  void open();
  void close();
  void load();
  void save();
  void save_session();
  void reset();

  std::string get_config_file() const { return config_file; }
  std::string get_session_file() const { return session_file; }

 public:
  int update_interval_slow = 200;
  double size_factor_slow = 0.004;
  int update_interval_fast = 25;
  double size_factor_fast = 0.001;
  std::string html_processor{"native"};
  std::string markdown_processor{"native"};
  std::string asciidoc_processor{"asciidoctor"};
  std::string fountain_processor{"screenplain"};
  std::string wiki_default{"mediawiki"};
  bool pandoc_disabled = false;
  bool pandoc_fragment = false;
  bool pandoc_toc = false;
  std::string pandoc_markdown{"markdown"};
  std::string default_font_family{"serif"};
  bool extended_types = true;
  int snippet_window = 5000;
  int snippet_trigger = 100000;
  bool snippet_html = false;
  bool snippet_markdown = true;
  bool snippet_asciidoctor = true;
  bool snippet_pandoc = true;
  bool snippet_screenplain = true;
  std::string extra_css{"disabled"};

 private:
  bool kf_has_key(std::string const &key);

  void kf_set_boolean(std::string const &key, bool const &val);
  bool kf_get_boolean(std::string const &key, bool const &def);

  void kf_set_double(std::string const &key, double const &val);
  double kf_get_double(std::string const &key, double const &def,
                       double const &min);

  void kf_set_integer(std::string const &key, int const &val);
  int kf_get_integer(std::string const &key, int const &def, int const &min);

  void kf_set_string(std::string const &key, std::string const &val);
  std::string kf_get_string(std::string const &key, std::string const &def);

 private:
  bool bSaveInProgress = false;
  GKeyFile *keyfile = nullptr;
  GKeyFile *session = nullptr;
  std::string config_file;
  std::string session_file;
};
