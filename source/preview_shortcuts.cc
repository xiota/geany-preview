// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_shortcuts.h"

#include <algorithm>  // std::transform
#include <cstddef>    // std::size
#include <cstdlib>    // std::getenv
#include <filesystem>
#include <iterator>
#include <sstream>
#include <string>

#include <gtk/gtk.h>
#include <keybindings.h>
#include <msgwindow.h>
#include <plugindata.h>
#include <ui_utils.h>

#include "preview_pane.h"
#include "subprocess.h"
#include "util/gtk_utils.h"
#include "util/string_utils.h"
#include "webview.h"

namespace {

PreviewContext *preview_context = nullptr;

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

}  // namespace

void PreviewShortcuts::onFocusPreview(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_) {
    return;
  }

  GtkWidget *preview = preview_context->preview_pane_->widget();
  if (preview) {
    GtkUtils::activateNotebookPageForWidget(preview);
  }
}

void PreviewShortcuts::onFocusPreviewEditor(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;
  GtkWidget *preview = preview_context->preview_pane_->widget();

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
  if (!preview_context || !preview_context->geany_sidebar_) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inSidebar =
      preview_context->geany_sidebar_ && gtk_widget_has_focus(preview_context->geany_sidebar_);

  if (!inEditor && !inSidebar) {
    keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
    return;
  }

  if (inEditor && preview_context->geany_sidebar_) {
    GtkUtils::activateNotebookPageForWidget(preview_context->geany_sidebar_);
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

  std::string cmd = preview_context->preview_config_->get<std::string>("terminal_command");

  if (!Subprocess::commandExists(cmd)) {
    if (Subprocess::commandExists("xdg-terminal-exec")) {
      cmd = "xdg-terminal-exec --working-directory=%d";
    } else {
      std::string de = detectDesktopEnv();
      std::string deTerm = pickTerminalForDe(de);
      if (!deTerm.empty() && Subprocess::commandExists(deTerm)) {
        cmd = deTerm;
      }
    }
  }

  if (!Subprocess::commandExists(cmd)) {
    msgwin_status_add("Preview: No suitable terminal found.");
    return;
  }

  cmd = StringUtils::replaceAll(std::move(cmd), "%d", dirPath.string());
  cmd = StringUtils::replaceAll(std::move(cmd), "%%", "%");

  if (!Subprocess::runAsync(cmd)) {
    msgwin_status_add("Preview: No suitable terminal found.");
    return;
  }
}

void PreviewShortcuts::onCopy(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_ || !preview_context->webview_) {
    return;
  }

  if (GtkUtils::hasFocusWithin(GTK_WIDGET(preview_context->preview_pane_->widget()))) {
    auto *wv = preview_context->webview_->widget();
    webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(wv), WEBKIT_EDITING_COMMAND_COPY);
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_CLIPBOARD, GEANY_KEYS_CLIPBOARD_COPY);
  }
}

void PreviewShortcuts::onCut(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_ || !preview_context->webview_) {
    return;
  }

  if (GtkUtils::hasFocusWithin(GTK_WIDGET(preview_context->preview_pane_->widget()))) {
    auto *wv = preview_context->webview_->widget();
    webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(wv), WEBKIT_EDITING_COMMAND_CUT);
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_CLIPBOARD, GEANY_KEYS_CLIPBOARD_CUT);
  }
}

void PreviewShortcuts::onPaste(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_ || !preview_context->webview_) {
    return;
  }

  if (GtkUtils::hasFocusWithin(GTK_WIDGET(preview_context->preview_pane_->widget()))) {
    auto *wv = preview_context->webview_->widget();
    webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(wv), WEBKIT_EDITING_COMMAND_PASTE);
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_CLIPBOARD, GEANY_KEYS_CLIPBOARD_PASTE);
  }
}

void PreviewShortcuts::onFind(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_ || !preview_context->webview_ ||
      !preview_context->geany_data_ || !preview_context->geany_data_->main_widgets ||
      !preview_context->geany_data_->main_widgets->window) {
    return;
  }

  if (GtkUtils::hasFocusWithin(GTK_WIDGET(preview_context->preview_pane_->widget()))) {
    preview_context->webview_->showFindPrompt(
        GTK_WINDOW(preview_context->geany_data_->main_widgets->window)
    );
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_FIND);
  }
}

void PreviewShortcuts::onFindNext(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_ || !preview_context->webview_ ||
      !preview_context->geany_data_ || !preview_context->geany_data_->main_widgets ||
      !preview_context->geany_data_->main_widgets->window) {
    return;
  }

  if (GtkUtils::hasFocusWithin(GTK_WIDGET(preview_context->preview_pane_->widget()))) {
    preview_context->webview_->findNext();
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_FINDNEXTSEL);
  }
}

void PreviewShortcuts::onFindPrev(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_ || !preview_context->webview_ ||
      !preview_context->geany_data_ || !preview_context->geany_data_->main_widgets ||
      !preview_context->geany_data_->main_widgets->window) {
    return;
  }

  if (GtkUtils::hasFocusWithin(GTK_WIDGET(preview_context->preview_pane_->widget()))) {
    preview_context->webview_->findPrevious();
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_FINDPREVSEL);
  }
}

void PreviewShortcuts::onFindWv(guint /*key_id*/) {
  if (!preview_context || !preview_context->webview_ || !preview_context->geany_data_ ||
      !preview_context->geany_data_->main_widgets ||
      !preview_context->geany_data_->main_widgets->window) {
    return;
  }

  preview_context->webview_->showFindPrompt(
      GTK_WINDOW(preview_context->geany_data_->main_widgets->window)
  );
}

void PreviewShortcuts::onFindNextWv(guint /*key_id*/) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  preview_context->webview_->findNext();
}

void PreviewShortcuts::onFindPrevWv(guint /*key_id*/) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  preview_context->webview_->findPrevious();
}

PreviewShortcuts::PreviewShortcuts(PreviewContext *context)
    : context_(context), key_group_(context_->geany_key_group_) {
  // Stash context for callbacks
  preview_context = context_;

  for (gsize i = 0; i < std::size(shortcut_defs_); ++i) {
    if (!shortcut_defs_[i].label) {
      break;  // stop at dummy entry
    }
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
