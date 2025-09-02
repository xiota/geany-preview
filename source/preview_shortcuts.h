// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iterator>

#include <keybindings.h>

#include "preview_context.h"

class PreviewShortcuts {
 public:
  explicit PreviewShortcuts(GeanyKeyGroup *group, PreviewContext *context)
      : group_(group), context_(context) {}

  void registerAll();

  static constexpr gsize shortcutCount() {
    return std::size(shortcut_defs_);
  }

 private:
  GeanyKeyGroup *group_;
  PreviewContext *context_;

  static void onFocusPreview(guint key_id);
  static void onFocusPreviewEditor(guint key_id);
  static void onFocusSidebarEditor(guint key_id);

  struct ShortcutDef {
    const char *label;
    const char *tooltip;
    GeanyKeyCallback callback;
  };

  inline static const ShortcutDef shortcut_defs_[] = {
      {"Focus Preview",           "Focus on preview", onFocusPreview      },

      {"Focus Preview or Editor",
       "Toggle focus between preview and editor",     onFocusPreviewEditor},
      {"Focus Sidebar or Editor",
       "Toggle focus between sidebar and editor",     onFocusSidebarEditor}
  };
};
