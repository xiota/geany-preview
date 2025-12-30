// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <webkit2/webkit2.h>

namespace WebViewContextMenu {

gboolean onContextMenu(
    WebKitWebView *view,
    WebKitContextMenu *menu,
    GdkEvent *event,
    WebKitHitTestResult *hit_test,
    gpointer user_data
);

}  // namespace WebViewContextMenu
