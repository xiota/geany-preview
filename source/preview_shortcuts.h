// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iterator>

#include <keybindings.h>

#include "preview_context.h"

class PreviewShortcuts {
 public:
  explicit PreviewShortcuts(PreviewContext *context)
      : context_(context), key_group_(context_->geany_key_group_) {}

  void registerAll();

  static constexpr gsize shortcutCount() {
    return std::size(shortcut_defs_);
  }

 private:
  PreviewContext *context_;
  GeanyKeyGroup *key_group_;

  static void onFocusPreview(guint key_id);
  static void onFocusPreviewEditor(guint key_id);
  static void onFocusSidebarEditor(guint key_id);

  struct ShortcutDef {
    const char *label;
    const char *tooltip;
    GeanyKeyCallback callback;
  };

  inline static const ShortcutDef shortcut_defs_[] = {
      {"Focus Preview",           "Focus on Preview", onFocusPreview      },

      {"Focus Preview or Editor",
       "Toggle focus between Preview and Editor",     onFocusPreviewEditor},
      {"Focus Sidebar or Editor",
       "Toggle focus between Sidebar and Editor",     onFocusSidebarEditor}
  };
};
