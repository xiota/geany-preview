// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iterator>

#include <keybindings.h>

#include "preview_context.h"

class PreviewShortcuts {
 public:
  explicit PreviewShortcuts(PreviewContext *context);

  static constexpr gsize shortcutCount() {
    return std::size(shortcut_defs_) - 1;
  }

 private:
  PreviewContext *context_;
  GeanyKeyGroup *key_group_;

  static void onCopy(guint /*key_id*/);
  static void onCut(guint /*key_id*/);
  static void onPaste(guint /*key_id*/);

  static void onFind(guint /*key_id*/);
  static void onFindNext(guint /*key_id*/);
  static void onFindPrev(guint /*key_id*/);

  static void onFindWv(guint /*key_id*/);
  static void onFindNextWv(guint /*key_id*/);
  static void onFindPrevWv(guint /*key_id*/);

  static void onFocusPreview(guint /*key_id*/);
  static void onFocusPreviewEditor(guint /*key_id*/);
  static void onFocusSidebarEditor(guint /*key_id*/);

  static void onOpenTerminal(guint /*key_id*/);
  static void onOpenFileManager(guint /*key_id*/);

  struct ShortcutDef {
    const char *label;
    const char *tooltip;
    GeanyKeyCallback callback;
  };

  // clang-format off
  inline static const ShortcutDef shortcut_defs_[] = {
    { "Copy in Preview (1)",
      "Copy selected text in Editor or Preview",
      onCopy },

    { "Copy in Preview (2)",
      "Extra copy shortcut (1).",
      onCopy },

    { "Copy in Preview (3)",
      "Extra copy shortcut (2)",
      onCopy },

    { "Cut in Preview (1)",
      "Cut selected text in Editor or Preview",
      onCut },

    { "Cut in Preview (2)",
      "Extra cut shortcut (1)",
      onCut },

    { "Cut in Preview (3)",
      "Extra cut shortcut (2)",
      onCut },

    { "Paste in Preview (1)",
      "Paste selected text in the Editor or Preview",
      onPaste },

    { "Paste in Preview (2)",
      "Extra paste shortcut (1)",
      onPaste },

    { "Paste in Preview (3)",
      "Extra paste shortcut (2)",
      onPaste },

    { "Find in Editor or Preview",
      "Open find prompt for Editor or Preview",
      onFind },

    { "Find in Preview",
      "Open find prompt for Preview only",
      onFindWv },

    { "Find Next in Editor or Preview",
      "Find next match in Editor or Preview",
      onFindNext },

    { "Find Next in Preview",
      "Find next match in Preview only",
      onFindNextWv },

    { "Find Previous in Editor or Preview",
      "Find previous match in Editor or Preview",
      onFindPrev },

    { "Find Previous in Preview",
      "Find previous match in Preview only",
      onFindPrevWv },

    { "Focus Preview",
      "Focus on Preview",
      onFocusPreview },

    { "Focus Preview or Editor",
      "Toggle focus between Preview and Editor",
      onFocusPreviewEditor },

    { "Focus Sidebar or Editor",
      "Toggle focus between Sidebar and Editor",
      onFocusSidebarEditor },

    { "Open Terminal",
      "Open Terminal at file location",
      onOpenTerminal },

    { "Open File Manager",
      "Open File Manager at file location",
      onOpenFileManager },

    { nullptr, nullptr, nullptr },
  };
  // clang-format on
};
