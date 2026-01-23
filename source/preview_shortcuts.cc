// SPDX-FileCopyrightText: Copyright 2025-2026 xiota
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
  auto &pane = PreviewPane::instance();
  GtkWidget *preview = pane.widget();

  auto &ctx = PreviewContext::instance();
  GtkWidget *sidebar = ctx.geany_sidebar_;
  if (!preview || !sidebar || !GTK_IS_NOTEBOOK(sidebar)) {
    return false;
  }

  return GtkUtils::isWidgetOnVisibleNotebookPage(GTK_NOTEBOOK(sidebar), preview);
}
}  // namespace

void PreviewShortcuts::onCopy(guint /*key_id*/) {
  auto &pane = PreviewPane::instance();
  if (GtkUtils::hasFocusWithin(GTK_WIDGET(pane.widget()))) {
    auto &wv = WebView::instance();
    webkit_web_view_execute_editing_command(
        WEBKIT_WEB_VIEW(wv.widget()), WEBKIT_EDITING_COMMAND_COPY
    );
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_CLIPBOARD, GEANY_KEYS_CLIPBOARD_COPY);
  }
}

void PreviewShortcuts::onCut(guint /*key_id*/) {
  auto &pane = PreviewPane::instance();
  if (GtkUtils::hasFocusWithin(GTK_WIDGET(pane.widget()))) {
    auto &wv = WebView::instance();
    webkit_web_view_execute_editing_command(
        WEBKIT_WEB_VIEW(wv.widget()), WEBKIT_EDITING_COMMAND_CUT
    );
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_CLIPBOARD, GEANY_KEYS_CLIPBOARD_CUT);
  }
}

void PreviewShortcuts::onPaste(guint /*key_id*/) {
  auto &pane = PreviewPane::instance();
  if (GtkUtils::hasFocusWithin(GTK_WIDGET(pane.widget()))) {
    auto &wv = WebView::instance();
    webkit_web_view_execute_editing_command(
        WEBKIT_WEB_VIEW(wv.widget()), WEBKIT_EDITING_COMMAND_PASTE
    );
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_CLIPBOARD, GEANY_KEYS_CLIPBOARD_PASTE);
  }
}

void PreviewShortcuts::onCopyFilePath(guint /*key_id*/) {
  GeanyDocument *doc = document_get_current();
  if (!DOC_VALID(doc) || !doc->real_path) {
    return;
  }

  const char *path = doc->real_path;

  GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  if (!clipboard) {
    return;
  }

  gtk_clipboard_set_text(clipboard, path, -1);
}

void PreviewShortcuts::onFind(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  auto &wv = WebView::instance();
  if (!ctx.geany_data_ || !ctx.geany_data_->main_widgets ||
      !ctx.geany_data_->main_widgets->window) {
    return;
  }

  auto &pane = PreviewPane::instance();
  if (GtkUtils::hasFocusWithin(GTK_WIDGET(pane.widget()))) {
    wv.showFindPrompt(GTK_WINDOW(ctx.geany_data_->main_widgets->window));
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_FIND);
  }
}

void PreviewShortcuts::onFindNext(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  auto &wv = WebView::instance();
  if (!ctx.geany_data_ || !ctx.geany_data_->main_widgets ||
      !ctx.geany_data_->main_widgets->window) {
    return;
  }

  auto &pane = PreviewPane::instance();
  if (GtkUtils::hasFocusWithin(GTK_WIDGET(pane.widget()))) {
    wv.findNext();
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_FINDNEXTSEL);
  }
}

void PreviewShortcuts::onFindPrev(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  auto &wv = WebView::instance();
  if (!ctx.geany_data_ || !ctx.geany_data_->main_widgets ||
      !ctx.geany_data_->main_widgets->window) {
    return;
  }

  auto &pane = PreviewPane::instance();
  if (GtkUtils::hasFocusWithin(GTK_WIDGET(pane.widget()))) {
    wv.findPrevious();
  } else {
    keybindings_send_command(GEANY_KEY_GROUP_SEARCH, GEANY_KEYS_SEARCH_FINDPREVSEL);
  }
}

void PreviewShortcuts::onFindWv(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  auto &wv = WebView::instance();
  if (!ctx.geany_data_ || !ctx.geany_data_->main_widgets ||
      !ctx.geany_data_->main_widgets->window) {
    return;
  }

  if (isPreviewVisible()) {
    wv.showFindPrompt(GTK_WINDOW(ctx.geany_data_->main_widgets->window));
  }
}

void PreviewShortcuts::onFindNextWv(guint /*key_id*/) {
  auto &wv = WebView::instance();
  if (isPreviewVisible()) {
    wv.findNext();
  }
}

void PreviewShortcuts::onFindPrevWv(guint /*key_id*/) {
  auto &wv = WebView::instance();
  if (isPreviewVisible()) {
    wv.findPrevious();
  }
}

void PreviewShortcuts::onFocusPreview(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  GtkWidget *sidebar = ctx.geany_sidebar_;
  if (!sidebar || !gtk_widget_get_visible(sidebar)) {
    return;
  }

  auto &pane = PreviewPane::instance();
  GtkWidget *preview = pane.widget();
  if (preview) {
    GtkUtils::activateNotebookPageForWidget(preview);
  }
}

void PreviewShortcuts::onFocusPreviewEditor(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  GtkWidget *sidebar = ctx.geany_sidebar_;
  if (!sidebar || !gtk_widget_get_visible(sidebar)) {
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;

  auto &pane = PreviewPane::instance();
  GtkWidget *preview = pane.widget();

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inPreview =
      preview && GtkUtils::hasFocusWithin(preview) && sidebar &&
      GtkUtils::isWidgetOnVisibleNotebookPage(GTK_NOTEBOOK(sidebar), preview);

  if (!inEditor && !inPreview) {
    auto &cfg = PreviewConfig::instance();
    bool strict = cfg.get<bool>("keybinding_behavior_strict", false);
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
  auto &ctx = PreviewContext::instance();
  GtkWidget *sidebar = ctx.geany_sidebar_;
  if (!sidebar || !gtk_widget_get_visible(sidebar)) {
    return;
  }

  GeanyDocument *doc = document_get_current();
  GtkWidget *sci = (doc && doc->editor) ? GTK_WIDGET(doc->editor->sci) : nullptr;

  const bool inEditor = sci && gtk_widget_has_focus(sci);
  const bool inSidebar = GtkUtils::hasFocusWithin(sidebar);

  if (!inEditor && !inSidebar) {
    auto &cfg = PreviewConfig::instance();
    bool strict = cfg.get<bool>("keybinding_behavior_strict", false);
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

  auto &cfg = PreviewConfig::instance();
  std::string cmd = cfg.get<std::string>("terminal_command");

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

  auto &cfg = PreviewConfig::instance();
  std::string cmd = cfg.get<std::string>("file_manager_command");

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

void PreviewShortcuts::onPreferences(guint /*key_id*/) {
  auto &ctx = PreviewContext::instance();
  ctx.openPreferences();
}

void PreviewShortcuts::onToggleSidebar(guint /*key_id*/) {
  keybindings_send_command(GEANY_KEY_GROUP_VIEW, GEANY_KEYS_VIEW_SIDEBAR);

  auto &ctx = PreviewContext::instance();
  GtkWidget *sidebar = ctx.geany_sidebar_;
  if (!sidebar) {
    return;
  }

  const bool visible = gtk_widget_get_visible(sidebar);
  if (visible) {
    GtkUtils::activateNotebookPageForWidget(sidebar);
  }
}

void PreviewShortcuts::onZoomInWv(guint /*key_id*/) {
  auto &wv = WebView::instance();
  if (isPreviewVisible()) {
    wv.stepZoom(+1);
  }
}

void PreviewShortcuts::onZoomOutWv(guint /*key_id*/) {
  auto &wv = WebView::instance();
  if (isPreviewVisible()) {
    wv.stepZoom(-1);
  }
}

void PreviewShortcuts::onResetZoomWv(guint /*key_id*/) {
  auto &wv = WebView::instance();
  if (isPreviewVisible()) {
    wv.resetZoom();
  }
}

void PreviewShortcuts::onZoomInBoth(guint /*key_id*/) {
  GeanyDocument *doc = document_get_current();
  ScintillaObject *sci = (DOC_VALID(doc) && doc->editor) ? doc->editor->sci : nullptr;

  auto &wv = WebView::instance();
  if (!sci) {
    return;
  }

  auto &cfg = PreviewConfig::instance();
  bool sync = cfg.get<bool>("preview_zoom_sync", false);

  if (sync) {
    scintilla_send_message(sci, SCI_ZOOMIN, 0, 0);
    wv.stepZoom(+1);
  } else if (gtk_widget_has_focus(GTK_WIDGET(sci))) {
    scintilla_send_message(sci, SCI_ZOOMIN, 0, 0);
  } else if (gtk_widget_has_focus(wv.widget())) {
    wv.stepZoom(+1);
  }
}

void PreviewShortcuts::onZoomOutBoth(guint /*key_id*/) {
  GeanyDocument *doc = document_get_current();
  ScintillaObject *sci = (DOC_VALID(doc) && doc->editor) ? doc->editor->sci : nullptr;

  if (!sci) {
    return;
  }

  auto &cfg = PreviewConfig::instance();
  bool sync = cfg.get<bool>("preview_zoom_sync", false);

  auto &wv = WebView::instance();
  if (sync) {
    scintilla_send_message(sci, SCI_ZOOMOUT, 0, 0);
    wv.stepZoom(-1);
  } else if (gtk_widget_has_focus(GTK_WIDGET(sci))) {
    scintilla_send_message(sci, SCI_ZOOMOUT, 0, 0);
  } else if (gtk_widget_has_focus(wv.widget())) {
    wv.stepZoom(-1);
  }
}

void PreviewShortcuts::onResetZoomBoth(guint /*key_id*/) {
  GeanyDocument *doc = document_get_current();
  ScintillaObject *sci = (DOC_VALID(doc) && doc->editor) ? doc->editor->sci : nullptr;

  if (!sci) {
    return;
  }

  auto &cfg = PreviewConfig::instance();
  bool sync = cfg.get<bool>("preview_zoom_sync", false);

  auto &wv = WebView::instance();
  if (sync) {
    scintilla_send_message(sci, SCI_SETZOOM, 0, 0);
    wv.resetZoom();
  } else if (gtk_widget_has_focus(GTK_WIDGET(sci))) {
    scintilla_send_message(sci, SCI_SETZOOM, 0, 0);
  } else if (gtk_widget_has_focus(wv.widget())) {
    wv.resetZoom();
  }
}
