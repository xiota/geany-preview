// SPDX-FileCopyrightText: Copyright 2021, 2025 xiota
// SPDX-FileCopyrightText: Copyright 2017 LarsGit223
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include <glib.h>
#include <gtk/gtk.h>
#include <pluginutils.h>
#include <scintilla/Scintilla.h>

#include "preview_config.h"
#include "preview_context.h"

class TweakUiColorTip {
 public:
  explicit TweakUiColorTip(PreviewContext *context) : context_(context) {
    // Connect button-press for already-open docs if chooser enabled
    if (colorChooserEnabled() && main_is_realized()) {
      auto *docs = context_->geany_data_->documents_array;
      for (guint i = 0; i < docs->len; ++i) {
        auto *doc = static_cast<GeanyDocument *>(g_ptr_array_index(docs, i));
        if (DOC_VALID(doc)) {
          connectDocumentButtonPressSignalHandler(doc);
        }
      }
    }

    // Hook into document/editor signals
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-open",
        true,
        G_CALLBACK(documentSignal),
        this
    );
    plugin_signal_connect(
        context_->geany_plugin_, nullptr, "document-new", true, G_CALLBACK(documentSignal), this
    );
    plugin_signal_connect(
        context_->geany_plugin_,
        nullptr,
        "document-close",
        false,
        G_CALLBACK(documentClose),
        this
    );
    plugin_signal_connect(
        context_->geany_plugin_, nullptr, "editor-notify", false, G_CALLBACK(editorNotify), this
    );

    setSize();
  }

  void setSize(std::string strSize = {}) {
    auto lowered = to_lower(trim(strSize));
    if (!lowered.empty()) {
      color_tooltip_size_ = lowered;
    }
    switch (color_tooltip_size_[0]) {
      case 'l':
        colortip_template_ = "          \n          \n          \n          ";
        break;
      case 'm':
        colortip_template_ = "        \n        ";
        break;
      case 's':
      default:
        colortip_template_ = "    ";
        break;
    }
  }

 private:
  bool colorTooltipEnabled() const {
    return context_ && context_->preview_config_ &&
           context_->preview_config_->get<bool>("color_tooltip", false);
  }
  bool colorChooserEnabled() const {
    return context_ && context_->preview_config_ &&
           context_->preview_config_->get<bool>("color_chooser", false);
  }

  static int containsColorValue(char *string, int position, int maxdist) {
    int color = -1;
    char *start = strchr(string, '#');
    if (!start) {
      start = strstr(string, "0x");
      if (start) {
        start += 1;
      }
    }
    if (!start) {
      return color;
    }

    int end = start - string + 1;
    while (g_ascii_isxdigit(string[end])) {
      end++;
    }
    end--;
    guint length = &(string[end]) - start + 1;

    if (maxdist != -1) {
      int offset = start - string + 1;
      if (position < offset && (offset - position) > maxdist) {
        return color;
      }
      offset = end;
      if (position > offset && (position - offset) > maxdist) {
        return color;
      }
    }

    if (length == 4) {
      start++;
      color = (g_ascii_xdigit_value(*start) << 4) | g_ascii_xdigit_value(*start);
      start++;
      int value = (g_ascii_xdigit_value(*start) << 4) | g_ascii_xdigit_value(*start);
      color += value << 8;
      start++;
      value = (g_ascii_xdigit_value(*start) << 4) | g_ascii_xdigit_value(*start);
      color += value << 16;
    } else if (length == 7) {
      start++;
      color = (g_ascii_xdigit_value(start[0]) << 4) | g_ascii_xdigit_value(start[1]);
      start += 2;
      int value = (g_ascii_xdigit_value(start[0]) << 4) | g_ascii_xdigit_value(start[1]);
      color += value << 8;
      start += 2;
      value = (g_ascii_xdigit_value(start[0]) << 4) | g_ascii_xdigit_value(start[1]);
      color += value << 16;
    }
    return color;
  }

  static int getColorValueAtCurrentDocPosition() {
    GeanyDocument *doc = document_get_current();
    g_return_val_if_fail(DOC_VALID(doc), -1);
    int color = -1;
    char *word = editor_get_word_at_pos(doc->editor, -1, "0123456789abcdefABCDEF");
    if (word) {
      switch (strlen(word)) {
        case 3:
          color = ((g_ascii_xdigit_value(word[0]) * 0x11) << 16) |
                  ((g_ascii_xdigit_value(word[1]) * 0x11) << 8) |
                  ((g_ascii_xdigit_value(word[2]) * 0x11) << 0);
          break;
        case 6:
          color = (g_ascii_xdigit_value(word[0]) << 20) |
                  (g_ascii_xdigit_value(word[1]) << 16) |
                  (g_ascii_xdigit_value(word[2]) << 12) | (g_ascii_xdigit_value(word[3]) << 8) |
                  (g_ascii_xdigit_value(word[4]) << 4) | (g_ascii_xdigit_value(word[5]) << 0);
          break;
        default:
          break;
      }
    }
    return color;
  }

  static gboolean
  onEditorButtonPressEvent(GtkWidget *, GdkEventButton *event, gpointer user_data) {
    auto *self = static_cast<TweakUiColorTip *>(user_data);
    if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
      if (!self->colorChooserEnabled()) {
        return FALSE;
      }
      if (getColorValueAtCurrentDocPosition() != -1) {
        keybindings_send_command(GEANY_KEY_GROUP_TOOLS, GEANY_KEYS_TOOLS_OPENCOLORCHOOSER);
      }
    }
    return FALSE;
  }

  void connectDocumentButtonPressSignalHandler(GeanyDocument *doc) {
    g_return_if_fail(DOC_VALID(doc));
    plugin_signal_connect(
        context_->geany_plugin_,
        G_OBJECT(doc->editor->sci),
        "button-press-event",
        false,
        G_CALLBACK(onEditorButtonPressEvent),
        this
    );
  }

  static void documentSignal(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiColorTip *>(user_data);
    g_return_if_fail(DOC_VALID(doc));
    self->connectDocumentButtonPressSignalHandler(doc);
    scintilla_send_message(doc->editor->sci, SCI_SETMOUSEDWELLTIME, 300, 0);
  }

  static void documentClose(GObject *, GeanyDocument *doc, gpointer user_data) {
    auto *self = static_cast<TweakUiColorTip *>(user_data);
    g_return_if_fail(DOC_VALID(doc));
    g_signal_handlers_disconnect_by_func(
        doc->editor->sci, gpointer(onEditorButtonPressEvent), self
    );
  }

  static bool
  editorNotify(GObject *, GeanyEditor *editor, SCNotification *nt, gpointer user_data) {
    auto *self = static_cast<TweakUiColorTip *>(user_data);
    ScintillaObject *sci = editor->sci;
    if (!self->colorTooltipEnabled()) {
      return false;
    }

    switch (nt->nmhdr.code) {
      case SCN_DWELLSTART: {
        if (nt->position < 0) {
          break;
        }
        int start = nt->position >= 7 ? nt->position - 7 : 0;
        int pos = nt->position - start;
        int end = nt->position + 7;
        int max = scintilla_send_message(sci, SCI_GETTEXTLENGTH, 0, 0);
        if (end > max) {
          end = max;
        }

        char *subtext = sci_get_contents_range(sci, start, end);
        if (subtext) {
          int color = containsColorValue(subtext, pos, 2);
          if (color != -1) {
            scintilla_send_message(sci, SCI_CALLTIPSETBACK, color, 0);
            scintilla_send_message(
                sci, SCI_CALLTIPSHOW, nt->position, (sptr_t)self->colortip_template_.c_str()
            );
          }
          g_free(subtext);
        }
      } break;
      case SCN_DWELLEND:
        scintilla_send_message(sci, SCI_CALLTIPCANCEL, 0, 0);
        break;
    }
    return false;
  }

  // Local helpers
  static std::string trim(std::string_view sv, std::string_view chars = " \t\n\r\f\v") {
    auto first = sv.find_first_not_of(chars);
    if (first == std::string_view::npos) {
      return {};
    }
    auto last = sv.find_last_not_of(chars);
    return std::string{sv.substr(first, last - first + 1)};
  }

  static std::string to_lower(std::string_view sv) {
    std::string out{sv};
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    return out;
  }

  PreviewContext *context_ = nullptr;
  std::string color_tooltip_size_{"small"};
  std::string colortip_template_{"    "};
};
