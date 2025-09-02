// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_shortcuts.h"

#include <algorithm>  // std::transform
#include <cstddef>    // std::size
#include <cstdlib>    // std::system, std::getenv
#include <filesystem>
#include <string>

#include <gtk/gtk.h>  // gtk_widget_grab_focus()
#include <keybindings.h>
#include <msgwindow.h>
#include <plugindata.h>
#include <ui_utils.h>

#include "preview_pane.h"
#include "util/gtk_helpers.h"

namespace {

PreviewContext *previewContext = nullptr;

bool commandExists(const std::string &cmd) {
  if (cmd.empty()) {
    return false;
  }

  // Extract the first token (the executable name)
  auto spacePos = cmd.find(' ');
  std::string exe = (spacePos == std::string::npos) ? cmd : cmd.substr(0, spacePos);

  std::string check = "command -v " + exe + " >/dev/null 2>&1";
  return (std::system(check.c_str()) == 0);
}

std::string detectDesktopEnv() {
  const char *env = std::getenv("XDG_CURRENT_DESKTOP");
  if (!env || !*env) {
    env = std::getenv("DESKTOP_SESSION");
  }
  if (!env) {
    return {};
  }

  std::string de = env;
  // Normalize to uppercase for easy compare
  std::transform(de.begin(), de.end(), de.begin(), ::toupper);
  return de;
}

std::string pickTerminalForDe(const std::string &de) {
  if (de.find("XFCE") != std::string::npos) {
    return "xfce4-terminal --working-directory=%d";
  }
  if (de.find("KDE") != std::string::npos || de.find("PLASMA") != std::string::npos) {
    return "konsole --workdir %d";
  }
  if (de.find("GNOME") != std::string::npos || de.find("UNITY") != std::string::npos) {
    return "gnome-terminal --working-directory=%d";
  }
  // Add more DEs if you want
  return {};
}

void replaceAll(std::string &s, const std::string &from, const std::string &to) {
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.length(), to);
    pos += to.length();
  }
}

}  // namespace

void PreviewShortcuts::onFocusPreview(guint /*key_id*/) {
  if (!previewContext || !previewContext->preview_pane_) {
    return;
  }

  GtkWidget *preview = previewContext->preview_pane_->widget();
  if (preview) {
    GtkUtils::activateNotebookPageForWidget(preview);
  }
}

void PreviewShortcuts::onFocusPreviewEditor(guint /*key_id*/) {
  if (!previewContext || !previewContext->preview_pane_) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;
  GtkWidget *preview = previewContext->preview_pane_->widget();

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inPreview = preview && gtk_widget_has_focus(preview);

  if (!inEditor && !inPreview) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  if (inEditor && preview) {
    GtkUtils::activateNotebookPageForWidget(preview);
    return;
  }

  keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
}

void PreviewShortcuts::onFocusSidebarEditor(guint /*key_id*/) {
  if (!previewContext || !previewContext->geany_sidebar_) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inSidebar =
      previewContext->geany_sidebar_ && gtk_widget_has_focus(previewContext->geany_sidebar_);

  if (!inEditor && !inSidebar) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  if (inEditor && previewContext->geany_sidebar_) {
    GtkUtils::activateNotebookPageForWidget(previewContext->geany_sidebar_);
    return;
  }

  keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
}

void PreviewShortcuts::onOpenTerminal(guint /*key_id*/) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc) || !doc->real_path) {
    return;
  }

  auto dirPath = std::filesystem::path(doc->real_path).parent_path();

  std::string cmd = previewContext->preview_config_->get<std::string>("terminal_command");

  if (!commandExists(cmd)) {
    if (commandExists("xdg-terminal-exec")) {
      cmd = "xdg-terminal-exec --working-directory=%d";
    } else {
      std::string de = detectDesktopEnv();
      std::string deTerm = pickTerminalForDe(de);
      if (!deTerm.empty() && commandExists(deTerm)) {
        cmd = deTerm;
      }
    }
  }

  if (!commandExists(cmd)) {
    msgwin_status_add("Preview: No suitable terminal found.");
    return;
  }

  replaceAll(cmd, "%%", "%");
  replaceAll(cmd, "%d", dirPath.string());

  std::ignore = std::system(cmd.c_str());
}

void PreviewShortcuts::registerAll() {
  // Stash context for callbacks
  previewContext = context_;

  for (gsize i = 0; i < std::size(shortcut_defs_); ++i) {
    keybindings_set_item(
        key_group_,
        i,
        shortcut_defs_[i].callback,
        0,  // no default key
        static_cast<GdkModifierType>(0),
        shortcut_defs_[i].label,
        shortcut_defs_[i].tooltip,
        nullptr /* user_data unused here */
    );
  }
}
