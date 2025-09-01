// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_shortcuts.h"

#include <geanyplugin.h>

PreviewShortcuts::PreviewShortcuts(GeanyKeyGroup *group) : group_(group) {}

void PreviewShortcuts::registerAll() {
  keybindings_set_item(
      group_,
      0,
      onRefresh,
      0,
      static_cast<GdkModifierType>(0),
      "Refresh Preview",
      "Refresh the preview pane",
      nullptr
  );
}

void PreviewShortcuts::onRefresh(guint /*key_id*/) {
  g_message("Preview refreshed via shortcut");
}
