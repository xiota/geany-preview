/*
 * WebView - Preview Plugin for Geany
 * Copyright 2021 xiota
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tkui_markword.h"

void PreviewWebView::initialize() {
  GEANY_PSC_AFTER("geany-startup-complete", document_signal);
  GEANY_PSC_AFTER("editor-notify", editor_notify);
  GEANY_PSC_AFTER("document-activate", document_signal);
  GEANY_PSC_AFTER("document-filetype-set", document_signal);
  GEANY_PSC_AFTER("document-new", document_signal);
  GEANY_PSC_AFTER("document-open", document_signal);
  GEANY_PSC_AFTER("document-reload", document_signal);
}

void PreviewWebView::document_signal(GObject *obj, GeanyDocument *doc,
                                     PreviewWebView *self) {}

bool PreviewWebView::editor_notify(GObject *object, GeanyEditor *editor,
                                   SCNotification *nt, PreviewWebView *self) {
  return false;
}
