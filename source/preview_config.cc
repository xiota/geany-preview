// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_config.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_set>

#include <gtk/gtk.h>
#include <toml++/toml.h>

namespace {

// Context for edited callback
struct edit_ctx_t {
  PreviewConfig *self;
  GtkTreeView *tree_view;
};

// used by buildConfigWidget, onValueEdited, onBoolToggled
enum {
  COL_KEY,          // string
  COL_TYPE,         // string
  COL_VALUE_STR,    // string (for int/string types)
  COL_VALUE_BOOL,   // bool (for bool types)
  COL_HELP,         // string (tooltip)
  COL_SHOW_TOGGLE,  // bool: show checkbox?
  COL_SHOW_TEXT,    // bool: show text cell?
  NUM_COLS
};

static void onBoolToggled(GtkCellRendererToggle *, gchar *path_str, gpointer user_data) {
  auto *ctx = static_cast<edit_ctx_t *>(user_data);
  GtkTreeModel *model = gtk_tree_view_get_model(ctx->tree_view);

  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_from_string(model, &iter, path_str)) {
    gboolean active;
    gchar *key_c;
    gtk_tree_model_get(model, &iter, COL_VALUE_BOOL, &active, COL_KEY, &key_c, -1);

    ctx->self->set<bool>(key_c, !active);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VALUE_BOOL, !active, -1);

    g_free(key_c);
  }
}

static std::string joinForDisplay(const std::vector<int> &v) {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) {
      out += ", ";
    }
    out += std::to_string(v[i]);
  }
  return out;
}

static std::string joinForDisplay(const std::vector<std::string> &v) {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) {
      out += ", ";
    }
    out += v[i];
  }
  return out;
}

static std::vector<std::string> splitFlexible(const std::string &input) {
  std::vector<std::string> result;
  std::string token;
  for (char ch : input) {
    if (ch == ',' || ch == ';' || ch == ':') {
      if (!token.empty()) {
        result.push_back(token);
        token.clear();
      }
    } else if (!std::isspace(static_cast<unsigned char>(ch))) {
      token.push_back(ch);
    }
  }
  if (!token.empty()) {
    result.push_back(token);
  }
  return result;
}

static std::vector<int> parseIntsFlexible(const std::string &input) {
  std::vector<int> out;
  for (auto &tok : splitFlexible(input)) {
    try {
      out.push_back(std::stoi(tok));
    } catch (...) { /* ignore invalid */
    }
  }
  return out;
}

static void
onValueEdited(GtkCellRendererText *, gchar *path_str, gchar *new_text, gpointer user_data) {
  auto *ctx = static_cast<edit_ctx_t *>(user_data);
  GtkTreeModel *model = gtk_tree_view_get_model(ctx->tree_view);

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_from_string(model, &iter, path_str)) {
    return;
  }

  gchar *key_c;
  gchar *type_c;
  gtk_tree_model_get(model, &iter, COL_KEY, &key_c, COL_TYPE, &type_c, -1);

  std::string key = key_c;
  std::string type = type_c;
  g_free(key_c);
  g_free(type_c);

  if (type == "int") {
    try {
      ctx->self->set<int>(key, std::stoi(new_text));
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VALUE_STR, new_text, -1);
    } catch (...) {
      g_warning("Invalid integer: %s", new_text);
    }
  } else if (type == "string") {
    ctx->self->set<std::string>(key, new_text);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VALUE_STR, new_text, -1);
  } else if (type == "vector<int>") {
    ctx->self->set<std::vector<int>>(key, parseIntsFlexible(new_text));
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VALUE_STR, new_text, -1);
  } else if (type == "vector<string>") {
    ctx->self->set<std::vector<std::string>>(key, splitFlexible(new_text));
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VALUE_STR, new_text, -1);
  }
}

}  // namespace

PreviewConfig::PreviewConfig(const std::filesystem::path &full_path) {
  std::filesystem::create_directories(full_path.parent_path());
  config_path_ = full_path.string();

  // Populate settings_ and help_texts_ from the master table
  for (const auto &def : setting_defs_) {
    settings_[def.key] = def.default_value;
    help_texts_[def.key] = def.help;
  }
}

bool PreviewConfig::load() {
  namespace fs = std::filesystem;
  if (!fs::exists(config_path_)) {
    return false;
  }

  try {
    auto tbl = toml::parse_file(config_path_);
    if (auto preview_tbl = tbl["Preview"].as_table()) {
      for (auto &[key, value] : settings_) {
        auto node = (*preview_tbl)[key];
        std::visit(
            [&](auto &current) {
              using T = std::decay_t<decltype(current)>;

              if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool> ||
                            std::is_same_v<T, std::string>) {
                if (auto v = node.value<T>()) {
                  current = *v;
                }
              } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                if (auto arr = node.as_array()) {
                  std::vector<int> tmp;
                  tmp.reserve(arr->size());
                  for (auto &el : *arr) {
                    if (auto iv = el.value<int>()) {
                      tmp.push_back(*iv);
                    }
                  }
                  current = std::move(tmp);
                }
              } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                if (auto arr = node.as_array()) {
                  std::vector<std::string> tmp;
                  tmp.reserve(arr->size());
                  for (auto &el : *arr) {
                    if (auto sv = el.value<std::string>()) {
                      tmp.push_back(*sv);
                    }
                  }
                  current = std::move(tmp);
                }
              }
            },
            value
        );
      }
    }
    return true;
  } catch (const toml::parse_error &err) {
    auto desc = err.description();
    g_warning("Failed to parse config: %.*s", static_cast<int>(desc.size()), desc.data());
    return false;
  }
}

bool PreviewConfig::save() const {
  toml::table preview_tbl;
  for (const auto &[key, value] : settings_) {
    std::visit(
        [&](auto &&current) {
          using T = std::decay_t<decltype(current)>;
          if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool> ||
                        std::is_same_v<T, std::string>) {
            preview_tbl.insert_or_assign(key, current);
          } else if constexpr (std::is_same_v<T, std::vector<int>> ||
                               std::is_same_v<T, std::vector<std::string>>) {
            toml::array arr;
            for (const auto &elem : current) {
              arr.push_back(elem);
            }
            preview_tbl.insert_or_assign(key, std::move(arr));
          }
        },
        value
    );
  }
  toml::table root;
  root.insert_or_assign("Preview", std::move(preview_tbl));

  try {
    std::ofstream out(config_path_, std::ios::trunc);
    out << root;
    return true;
  } catch (...) {
    g_warning("Failed to save config to %s", config_path_.c_str());
    return false;
  }
}

void PreviewConfig::onDialogResponse(GtkDialog *dialog, gint response_id) {
  switch (response_id) {
    case GTK_RESPONSE_APPLY:
      save();
      break;
    case GTK_RESPONSE_OK:
      save();
      gtk_widget_destroy(GTK_WIDGET(dialog));
      break;
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy(GTK_WIDGET(dialog));
      break;
  }
}

GtkWidget *PreviewConfig::buildConfigWidget(GtkDialog *dialog) {
  GtkListStore *store = gtk_list_store_new(
      NUM_COLS,
      G_TYPE_STRING,   // key
      G_TYPE_STRING,   // type
      G_TYPE_STRING,   // value as string (for int/string)
      G_TYPE_BOOLEAN,  // value as bool (for bool type)
      G_TYPE_STRING,   // help text (tooltip)
      G_TYPE_BOOLEAN,  // show toggle?
      G_TYPE_BOOLEAN   // show text?
  );

  // Populate from setting_defs_ to preserve order
  for (const auto &def : setting_defs_) {
    auto it = settings_.find(def.key);
    if (it == settings_.end()) {
      continue;
    }

    const auto &val = it->second;
    std::string type, value_str;
    gboolean value_bool = FALSE;

    std::visit(
        [&](auto &&v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, int>) {
            type = "int";
            value_str = std::to_string(v);
          } else if constexpr (std::is_same_v<T, bool>) {
            type = "bool";
            value_bool = v;
          } else if constexpr (std::is_same_v<T, std::vector<int>>) {
            type = "vector<int>";
            value_str = joinForDisplay(v);
          } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            type = "vector<string>";
            value_str = joinForDisplay(v);
          } else {
            type = "string";
            value_str = v;
          }
        },
        val
    );

    bool is_bool = (type == "bool");

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(
        store,
        &iter,
        COL_KEY,
        def.key,
        COL_TYPE,
        type.c_str(),
        COL_VALUE_STR,
        value_str.c_str(),
        COL_VALUE_BOOL,
        value_bool,
        COL_HELP,
        def.help,
        COL_SHOW_TOGGLE,
        is_bool,
        COL_SHOW_TEXT,
        !is_bool,
        -1
    );
  }

  GtkWidget *tree_view_widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  GtkTreeView *tree_view = GTK_TREE_VIEW(tree_view_widget);
  g_object_unref(store);

  // Tooltips from hidden help column
  gtk_tree_view_set_tooltip_column(tree_view, COL_HELP);

  auto *ctx = g_new(edit_ctx_t, 1);
  ctx->self = this;
  ctx->tree_view = tree_view;

  // Column: Key (read-only text)
  {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes("Key", renderer, "text", COL_KEY, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(tree_view, column);
  }

  // Column: Value (toggle for bool, text for others)
  {
    GtkTreeViewColumn *value_column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(value_column, "Value");

    // Toggle renderer for bools
    GtkCellRenderer *toggle_renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(toggle_renderer, "toggled", G_CALLBACK(onBoolToggled), ctx);
    gtk_tree_view_column_pack_start(value_column, toggle_renderer, FALSE);
    gtk_tree_view_column_add_attribute(value_column, toggle_renderer, "active", COL_VALUE_BOOL);
    gtk_tree_view_column_add_attribute(
        value_column, toggle_renderer, "visible", COL_SHOW_TOGGLE
    );

    // Text renderer for int/string
    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
    g_object_set(text_renderer, "editable", TRUE, NULL);
    g_signal_connect_data(
        text_renderer,
        "edited",
        G_CALLBACK(onValueEdited),
        ctx,
        (GClosureNotify)g_free,
        (GConnectFlags)0
    );
    ctx = nullptr;  // ownership passed to signal
    gtk_tree_view_column_pack_start(value_column, text_renderer, TRUE);
    gtk_tree_view_column_add_attribute(value_column, text_renderer, "text", COL_VALUE_STR);
    gtk_tree_view_column_add_attribute(value_column, text_renderer, "visible", COL_SHOW_TEXT);

    gtk_tree_view_append_column(tree_view, value_column);
  }

  // Column: Type (read-only text)
  {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes("Type", renderer, "text", COL_TYPE, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(tree_view, column);
  }

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(tree_view));
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
  );
  gtk_widget_set_size_request(scrolled_window, 500, 250);

  g_signal_connect(
      dialog,
      "response",
      G_CALLBACK(+[](GtkDialog *d, gint id, gpointer ud) {
        static_cast<PreviewConfig *>(ud)->onDialogResponse(d, id);
      }),
      this
  );

  return scrolled_window;
}
