/*
 * Settings - Preview Plugin for Geany
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

#include <vector>

#include "pv_main.h"

#define PREVIEW_KF_GROUP "preview"

typedef void tkuiSetting;

enum tkuiSettingType {
  TKUI_PREF_TYPE_NONE,
  TKUI_PREF_TYPE_BOOLEAN,
  TKUI_PREF_TYPE_INTEGER,
  TKUI_PREF_TYPE_DOUBLE,
  TKUI_PREF_TYPE_STRING,
};

class tkuiSettingPref {
 public:
  tkuiSettingType type;
  std::string name;
  std::string comment;
  bool session;
  tkuiSetting *setting;
};

class PreviewSettings {
 public:
  void initialize();

  void load();
  void save(bool bSession = false);
  void save_session();
  void reset();
  void kf_open();
  void kf_close();

 public:
  std::string config_file;

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

  void add_setting(tkuiSetting *setting, tkuiSettingType const &type,
                   std::string const &name, std::string const &comment,
                   bool const &session);

 private:
  GKeyFile *keyfile = nullptr;
  std::vector<tkuiSettingPref> pref_entries;
};
