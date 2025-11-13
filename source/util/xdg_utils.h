// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <unordered_set>

namespace XdgUtils {

inline const std::unordered_set<std::string> xdg_vars = {
  "PWD",
  "HOME",
  "XDG_CONFIG_HOME",
  "XDG_CACHE_HOME",
  "XDG_DATA_HOME",
  "XDG_STATE_HOME",
  "XDG_RUNTIME_DIR",
  "XDG_DESKTOP_DIR",
  "XDG_DOWNLOAD_DIR",
  "XDG_DOCUMENTS_DIR",
  "XDG_MUSIC_DIR",
  "XDG_PICTURES_DIR",
  "XDG_VIDEOS_DIR",
  "XDG_PUBLICSHARE_DIR",
  "XDG_TEMPLATES_DIR",
};

// Expand environment variables in a string.
// - Supports $VAR and ${VAR} forms.
// - If allowed_vars is non-empty, only expands those names.
// - If allowed_vars is empty, expands everything found.
std::string expandEnvVars(
    const std::string &input,
    const std::unordered_set<std::string> &allowed_vars = xdg_vars
);

}  // namespace XdgUtils
