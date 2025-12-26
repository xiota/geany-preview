// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "preview_context.h"
#include "webview.h"

class WebViewFindDialog {
 public:
  WebViewFindDialog(WebView *wv, PreviewContext *ctx);
  ~WebViewFindDialog();

  void show(GtkWindow *parent);
  void attachAutoDismissFor(GtkWidget *widget);

 private:
  void buildDialog(GtkWindow *parent);

  GtkWidget *dialog_ = nullptr;
  GtkWidget *entry_ = nullptr;

  WebView *webview_ = nullptr;
  PreviewContext *context_ = nullptr;

  gulong auto_dismiss_handler_ = 0;
  gulong notebook_switch_handler_ = 0;

  GtkWidget *notebook_ = nullptr;
};
