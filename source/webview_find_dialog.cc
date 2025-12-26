// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webview_find_dialog.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "preview_context.h"
#include "webview.h"

WebViewFindDialog::WebViewFindDialog(WebView *wv, PreviewContext *ctx)
    : webview_(wv), context_(ctx) {}

WebViewFindDialog::~WebViewFindDialog() {
  if (dialog_) {
    gtk_widget_destroy(dialog_);
    dialog_ = nullptr;
  }
}

void WebViewFindDialog::show(GtkWindow *parent) {
  if (dialog_) {
    gtk_window_present(GTK_WINDOW(dialog_));
    return;
  }

  buildDialog(parent);
  gtk_widget_show_all(dialog_);

  // Center over WebView
  if (gtk_widget_get_window(webview_->widget())) {
    int wx = 0, wy = 0;
    gdk_window_get_origin(gtk_widget_get_window(webview_->widget()), &wx, &wy);

    int w_width = gtk_widget_get_allocated_width(webview_->widget());
    int w_height = gtk_widget_get_allocated_height(webview_->widget());

    GtkRequisition req;
    gtk_widget_get_preferred_size(dialog_, &req, nullptr);
    int f_width = req.width;
    int f_height = req.height;

    int pos_x = wx + (w_width - f_width) / 2;
    int pos_y = wy + (w_height - f_height) / 2;

    gtk_window_move(GTK_WINDOW(dialog_), pos_x, pos_y);
  }
}

void WebViewFindDialog::buildDialog(GtkWindow *parent) {
  dialog_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dialog_), "Find in Page");
  gtk_window_set_transient_for(GTK_WINDOW(dialog_), parent);
  gtk_window_set_modal(GTK_WINDOW(dialog_), false);
  gtk_container_set_border_width(GTK_CONTAINER(dialog_), 6);

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add(GTK_CONTAINER(dialog_), hbox);

  entry_ = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_), "Search text…");
  gtk_entry_set_activates_default(GTK_ENTRY(entry_), true);

  GtkWidget *btn_next = gtk_button_new_with_label("Next");
  GtkWidget *btn_prev = gtk_button_new_with_label("Previous");

  gtk_box_pack_start(GTK_BOX(hbox), entry_, true, true, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btn_next, false, false, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btn_prev, false, false, 0);

  // Get find controller
  WebKitFindController *fc =
      webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(webview_->widget()));

  // Entry changed → search or clear
  g_signal_connect(
      entry_,
      "changed",
      G_CALLBACK(+[](GtkEntry *entry, gpointer data) {
        auto *fc = static_cast<WebKitFindController *>(data);
        const char *text = gtk_entry_get_text(entry);
        if (text && *text) {
          webkit_find_controller_search(
              fc,
              text,
              WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE | WEBKIT_FIND_OPTIONS_WRAP_AROUND,
              G_MAXUINT
          );
        } else {
          webkit_find_controller_search_finish(fc);
        }
      }),
      fc
  );

  // Enter key → find next
  g_signal_connect(
      entry_,
      "activate",
      G_CALLBACK(+[](GtkEntry *, gpointer data) {
        webkit_find_controller_search_next(static_cast<WebKitFindController *>(data));
      }),
      fc
  );

  // Buttons
  g_signal_connect(
      btn_next,
      "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        webkit_find_controller_search_next(static_cast<WebKitFindController *>(data));
      }),
      fc
  );

  g_signal_connect(
      btn_prev,
      "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        webkit_find_controller_search_previous(static_cast<WebKitFindController *>(data));
      }),
      fc
  );

  // Escape key closes
  g_signal_connect(
      dialog_,
      "key-press-event",
      G_CALLBACK(+[](GtkWidget *widget, GdkEventKey *event, gpointer) -> gboolean {
        if (event->keyval == GDK_KEY_Escape) {
          gtk_widget_destroy(widget);
          return true;
        }
        return false;
      }),
      nullptr
  );

  // Destroy → clear search + null dialog_
  g_signal_connect(
      dialog_,
      "destroy",
      G_CALLBACK(+[](GtkWidget *, gpointer data) {
        auto *self = static_cast<WebViewFindDialog *>(data);
        WebKitFindController *fc =
            webkit_web_view_get_find_controller(WEBKIT_WEB_VIEW(self->webview_->widget()));
        webkit_find_controller_search_finish(fc);
        self->dialog_ = nullptr;
      }),
      this
  );

  // Destroy dialog if WebView is destroyed
  g_signal_connect(
      webview_->widget(),
      "destroy",
      G_CALLBACK(+[](GtkWidget *, gpointer data) {
        GtkWidget **p = static_cast<GtkWidget **>(data);
        if (*p && GTK_IS_WIDGET(*p)) {
          gtk_widget_destroy(*p);
        }
        *p = nullptr;
      }),
      &dialog_
  );

  // Auto-null when dialog is finalized
  g_object_add_weak_pointer(G_OBJECT(dialog_), reinterpret_cast<gpointer *>(&dialog_));
}
