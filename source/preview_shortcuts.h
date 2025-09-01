// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <geanyplugin.h>

/**
 * @file preview_shortcuts.h
 * @brief Handles keyboard shortcut registration and callbacks for the preview plugin.
 */

class PreviewShortcuts {
 public:
  explicit PreviewShortcuts(GeanyKeyGroup *group);
  void registerAll();

 private:
  static void onRefresh(guint key_id);
  GeanyKeyGroup *group_;
};
