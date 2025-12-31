// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webview_context_menu.h"

#include <gtk/gtk.h>
#include <jsc/jsc.h>

#include "preview_context.h"
#include "webview.h"
#include "webview_find_dialog.h"

namespace {

// Remove unwanted default WebKit menu items
void removeUnwantedItems(WebKitContextMenu *menu) {
  if (GList *items = webkit_context_menu_get_items(menu)) {
    for (GList *l = items; l != nullptr;) {
      WebKitContextMenuItem *item = static_cast<WebKitContextMenuItem *>(l->data);
      WebKitContextMenuAction action = webkit_context_menu_item_get_stock_action(item);

      l = l->next;

      if (action == WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_GO_BACK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD ||
          action == WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW ||
          action == WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK ||
          action == WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW ||
          action == WEBKIT_CONTEXT_MENU_ACTION_RELOAD ||
          action == WEBKIT_CONTEXT_MENU_ACTION_STOP) {
        webkit_context_menu_remove(menu, item);
      }
    }
  }
}

// Reload File
void onReloadActivate(GSimpleAction *, GVariant *, gpointer) {
  keybindings_send_command(GEANY_KEY_GROUP_FILE, GEANY_KEYS_FILE_RELOAD);
}

WebKitContextMenuItem *createReloadItem() {
  GSimpleAction *action = g_simple_action_new("reload_file", nullptr);
  g_signal_connect(action, "activate", G_CALLBACK(onReloadActivate), nullptr);

  WebKitContextMenuItem *item =
      webkit_context_menu_item_new_from_gaction(G_ACTION(action), "Reload File", nullptr);

  g_object_unref(action);
  return item;
}

// Find in Page
void onFindActivate(GSimpleAction *, GVariant *, gpointer data) {
  auto *wv = static_cast<WebView *>(data);
  if (!wv) {
    return;
  }

  GtkWidget *toplevel = gtk_widget_get_toplevel(wv->widget());
  GtkWindow *parent = GTK_IS_WINDOW(toplevel) ? GTK_WINDOW(toplevel) : nullptr;

  // Open find dialog immediately
  wv->showFindPrompt(parent);

  // Async JS to get selected text
  webkit_web_view_evaluate_javascript(
      WEBKIT_WEB_VIEW(wv->widget()),
      "window.getSelection().toString();",
      -1,
      nullptr,
      nullptr,
      nullptr,
      [](GObject *obj, GAsyncResult *res, gpointer user_data) {
        auto *wv = static_cast<WebView *>(user_data);

        GError *err = nullptr;
        JSCValue *value =
            webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(obj), res, &err);

        if (err) {
          g_error_free(err);
          return;
        }

        gchar *selected = nullptr;
        if (jsc_value_is_string(value)) {
          selected = jsc_value_to_string(value);
        }

        if (wv->getFindDialog()) {
          GtkWidget *entry = wv->getFindDialog()->entry();
          if (entry && selected && *selected) {
            gtk_entry_set_text(GTK_ENTRY(entry), selected);
          }
        }

        if (selected) {
          g_free(selected);
        }
        if (G_IS_OBJECT(value)) {
          g_object_unref(value);
        }
      },
      wv
  );
}

WebKitContextMenuItem *createFindItem() {
  auto &ctx = PreviewContext::instance();

  if (!ctx.webview_) {
    return nullptr;
  }

  GSimpleAction *action = g_simple_action_new("find_in_page", nullptr);
  g_signal_connect(action, "activate", G_CALLBACK(onFindActivate), ctx.webview_);

  WebKitContextMenuItem *item =
      webkit_context_menu_item_new_from_gaction(G_ACTION(action), "Find in Page…", nullptr);

  g_object_unref(action);
  return item;
}

// Preferences
void onPrefsActivate(GSimpleAction *, GVariant *, gpointer data) {
  auto &ctx = PreviewContext::instance();
  ctx.openPreferences();
}

WebKitContextMenuItem *createPreferencesItem() {
  GSimpleAction *action = g_simple_action_new("open_preferences", nullptr);
  g_signal_connect(action, "activate", G_CALLBACK(onPrefsActivate), nullptr);

  WebKitContextMenuItem *item =
      webkit_context_menu_item_new_from_gaction(G_ACTION(action), "Preferences", nullptr);

  g_object_unref(action);
  return item;
}

}  // namespace

namespace WebViewContextMenu {

gboolean onContextMenu(
    WebKitWebView *,
    WebKitContextMenu *menu,
    GdkEvent *,
    WebKitHitTestResult *,
    gpointer user_data
) {
  /*
  bool is_link = webkit_hit_test_result_context_is_link(hit_test);
  bool is_image = webkit_hit_test_result_context_is_image(hit_test);
  bool is_media = webkit_hit_test_result_context_is_media(hit_test);
  bool is_select = webkit_hit_test_result_context_is_selection(hit_test);
  bool is_edit = webkit_hit_test_result_context_is_editable(hit_test);
  */

  removeUnwantedItems(menu);

  webkit_context_menu_append(menu, webkit_context_menu_item_new_separator());
  webkit_context_menu_append(menu, createReloadItem());

  webkit_context_menu_append(menu, createFindItem());

  webkit_context_menu_append(menu, createPreferencesItem());

  return false;
}

}  // namespace WebViewContextMenu
