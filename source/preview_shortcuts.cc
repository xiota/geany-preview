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

#include <Scintilla.h>  // for SCI_* messages
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

struct DeCommand {
  const char *key;      // substring to look for in DE string
  const char *command;  // associated command template
};

constexpr DeCommand terminalCommands[] = {
  { "XFCE", "xfce4-terminal --working-directory=%d" },
  { "KDE", "konsole --workdir %d" },
  { "GNOME", "gnome-terminal --working-directory=%d" },
  { nullptr, nullptr }  // sentinel
};

constexpr DeCommand fileManagerCommands[] = {
  { "XFCE", "thunar %d" },
  { "KDE", "dolphin %d" },
  { "GNOME", "nautilus %d" },
  { nullptr, nullptr }  // sentinel
};

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

std::string pickCommandForDe(const std::string &de, const DeCommand *cmds) {
  for (const DeCommand *c = cmds; c->key != nullptr; ++c) {
    if (de.find(c->key) != std::string::npos) {
      return c->command;
    }
  }
  return {};
}

bool isPreviewVisible() {
  if (!preview_context || !preview_context->preview_pane_) {
    return false;
  }

  GtkWidget *preview = preview_context->preview_pane_->widget();
  GtkWidget *sidebar = preview_context->geany_sidebar_;
  if (!preview || !sidebar || !GTK_IS_NOTEBOOK(sidebar)) {
    return false;
  }

  return GtkUtils::isWidgetOnVisibleNotebookPage(GTK_NOTEBOOK(sidebar), preview);
}
}  // namespace

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

void PreviewShortcuts::onFocusPreview(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_) {
    return;
  }

  GtkWidget *sidebar = preview_context->geany_sidebar_;
  if (!sidebar || !gtk_widget_get_visible(sidebar)) {
    return;
  }

  GtkWidget *preview = preview_context->preview_pane_->widget();
  if (preview) {
    GtkUtils::activateNotebookPageForWidget(preview);
  }
}

void PreviewShortcuts::onFocusPreviewEditor(guint /*key_id*/) {
  if (!preview_context || !preview_context->preview_pane_) {
    return;
  }

  GtkWidget *sidebar = preview_context->geany_sidebar_;
  if (!sidebar || !gtk_widget_get_visible(sidebar)) {
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;
  GtkWidget *preview = preview_context->preview_pane_->widget();

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inPreview =
      preview && GtkUtils::hasFocusWithin(preview) && sidebar &&
      GtkUtils::isWidgetOnVisibleNotebookPage(GTK_NOTEBOOK(sidebar), preview);

  if (!inEditor && !inPreview) {
    bool strict =
        preview_context->preview_config_->get<bool>("keybinding_behavior_strict", false);
    if (strict) {
      return;
    } else {
      keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
      return;
    }
  }

  if (inEditor && preview) {
    GtkUtils::activateNotebookPageForWidget(preview);
    return;
  }

  keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
}

void PreviewShortcuts::onFocusSidebarEditor(guint /*key_id*/) {
  if (!preview_context) {
    return;
  }

  GtkWidget *sidebar = preview_context->geany_sidebar_;
  if (!sidebar || !gtk_widget_get_visible(sidebar)) {
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inSidebar = GtkUtils::hasFocusWithin(sidebar);

  if (!inEditor && !inSidebar) {
    bool strict =
        preview_context->preview_config_->get<bool>("keybinding_behavior_strict", false);
    if (strict) {
      return;
    } else {
      keybindings_send_command(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_EDITOR);
      return;
    }
  }

  if (inEditor && sidebar) {
    GtkUtils::activateNotebookPageForWidget(sidebar);
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
      std::string deTerm = pickCommandForDe(de, terminalCommands);
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

void PreviewShortcuts::onOpenFileManager(guint /*key_id*/) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc) || !doc->real_path) {
    return;
  }

  auto dirPath = std::filesystem::path(doc->real_path).parent_path();

  std::string cmd = preview_context->preview_config_->get<std::string>("file_manager_command");

  if (!Subprocess::commandExists(cmd)) {
    if (Subprocess::commandExists("xdg-open")) {
      cmd = "xdg-open %d";
    } else {
      std::string de = detectDesktopEnv();
      std::string deCmd = pickCommandForDe(de, fileManagerCommands);
      if (!deCmd.empty() && Subprocess::commandExists(deCmd)) {
        cmd = deCmd;
      }
    }
  }

  if (!Subprocess::commandExists(cmd)) {
    msgwin_status_add("Preview: No suitable file manager found.");
    return;
  }

  cmd = StringUtils::replaceAll(std::move(cmd), "%d", dirPath.string());
  cmd = StringUtils::replaceAll(std::move(cmd), "%%", "%");

  if (!Subprocess::runAsync(cmd)) {
    msgwin_status_add("Preview: No suitable file manager found.");
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

  if (isPreviewVisible()) {
    preview_context->webview_->showFindPrompt(
        GTK_WINDOW(preview_context->geany_data_->main_widgets->window)
    );
  }
}

void PreviewShortcuts::onFindNextWv(guint /*key_id*/) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  if (isPreviewVisible()) {
    preview_context->webview_->findNext();
  }
}

void PreviewShortcuts::onFindPrevWv(guint /*key_id*/) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  if (isPreviewVisible()) {
    preview_context->webview_->findPrevious();
  }
}

void PreviewShortcuts::onZoomInWv(guint) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  if (isPreviewVisible()) {
    preview_context->webview_->stepZoom(+1);
  }
}

void PreviewShortcuts::onZoomOutWv(guint) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  if (isPreviewVisible()) {
    preview_context->webview_->stepZoom(-1);
  }
}

void PreviewShortcuts::onResetZoomWv(guint) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  if (isPreviewVisible()) {
    preview_context->webview_->resetZoom();
  }
}

void PreviewShortcuts::onZoomInBoth(guint) {
  if (!preview_context) {
    return;
  }
  bool sync = preview_context->preview_config_->get<bool>("preview_zoom_sync", false);

  GeanyDocument *doc = document_get_current();
  ScintillaObject *sci = (DOC_VALID(doc) && doc->editor) ? doc->editor->sci : nullptr;

  if (sync && sci) {
    scintilla_send_message(sci, SCI_ZOOMIN, 0, 0);
    preview_context->webview_->stepZoom(+1);
  } else if (sci && gtk_widget_has_focus(GTK_WIDGET(sci))) {
    scintilla_send_message(sci, SCI_ZOOMIN, 0, 0);
  } else if (gtk_widget_has_focus(preview_context->webview_->widget())) {
    preview_context->webview_->stepZoom(+1);
  }
}

void PreviewShortcuts::onZoomOutBoth(guint) {
  if (!preview_context) {
    return;
  }
  bool sync = preview_context->preview_config_->get<bool>("preview_zoom_sync", false);

  GeanyDocument *doc = document_get_current();
  ScintillaObject *sci = (DOC_VALID(doc) && doc->editor) ? doc->editor->sci : nullptr;

  if (sync && sci) {
    scintilla_send_message(sci, SCI_ZOOMOUT, 0, 0);
    preview_context->webview_->stepZoom(-1);
  } else if (sci && gtk_widget_has_focus(GTK_WIDGET(sci))) {
    scintilla_send_message(sci, SCI_ZOOMOUT, 0, 0);
  } else if (gtk_widget_has_focus(preview_context->webview_->widget())) {
    preview_context->webview_->stepZoom(-1);
  }
}

void PreviewShortcuts::onResetZoomBoth(guint) {
  if (!preview_context || !preview_context->webview_) {
    return;
  }

  bool sync = preview_context->preview_config_->get<bool>("preview_zoom_sync", false);

  GeanyDocument *doc = document_get_current();
  ScintillaObject *sci = (DOC_VALID(doc) && doc->editor) ? doc->editor->sci : nullptr;

  if (sync && sci) {
    scintilla_send_message(sci, SCI_SETZOOM, 0, 0);
    preview_context->webview_->resetZoom();
  } else if (sci && gtk_widget_has_focus(GTK_WIDGET(sci))) {
    scintilla_send_message(sci, SCI_SETZOOM, 0, 0);
  } else if (gtk_widget_has_focus(preview_context->webview_->widget())) {
    preview_context->webview_->resetZoom();
  }
}
