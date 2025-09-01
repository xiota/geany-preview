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

static void
onValueEdited(GtkCellRendererText *, gchar *path_str, gchar *new_text, gpointer user_data) {
  auto *ctx = static_cast<edit_ctx_t *>(user_data);
  PreviewConfig *self = ctx->self;
  GtkTreeModel *model = gtk_tree_view_get_model(ctx->tree_view);

  enum { COL_KEY, COL_TYPE, COL_VALUE };
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_from_string(model, &iter, path_str)) {
    return;
  }

  gchar *key_c = nullptr, *type_c = nullptr;
  gtk_tree_model_get(model, &iter, COL_KEY, &key_c, COL_TYPE, &type_c, -1);
  std::string key = key_c ? key_c : "";
  std::string type = type_c ? type_c : "";
  g_free(key_c);
  g_free(type_c);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VALUE, new_text, -1);

  if (type == "int") {
    try {
      self->set<int>(key, std::stoi(new_text));
    } catch (...) {
    }
  } else if (type == "bool") {
    std::string lower = new_text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    static const std::unordered_set<std::string> true_values = {
        "true", "t", "yes", "y", "on", "1"
    };
    static const std::unordered_set<std::string> false_values = {
        "false", "f", "no", "n", "off", "0"
    };

    bool parsed_value = false;
    if (true_values.count(lower)) {
      parsed_value = true;
    } else if (false_values.count(lower)) {
      parsed_value = false;
    } else {
      // revert UI cell to previous value or show an error
      return;
    }

    self->set<bool>(key, parsed_value);
  } else {
    self->set<std::string>(key, new_text);
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
              if (auto v = node.value<T>()) {
                current = *v;
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
        [&](auto &&current) { preview_tbl.insert_or_assign(key, toml::value{current}); }, value
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
  enum { COL_KEY, COL_TYPE, COL_VALUE, COL_HELP, NUM_COLS };
  GtkListStore *store =
      gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  // Iterate over setting_defs_ to preserve defined order
  for (const auto &def : setting_defs_) {
    const std::string key = def.key;

    // Look up current value in settings_
    auto it = settings_.find(key);
    if (it == settings_.end()) {
      continue;  // shouldn't happen if constructor populated settings_
    }

    const auto &val = it->second;
    std::string type, value;
    std::visit(
        [&](auto &&v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, int>) {
            type = "int";
            value = std::to_string(v);
          } else if constexpr (std::is_same_v<T, bool>) {
            type = "bool";
            value = v ? "true" : "false";
          } else {
            type = "string";
            value = v;
          }
        },
        val
    );

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(
        store,
        &iter,
        COL_KEY,
        key.c_str(),
        COL_TYPE,
        type.c_str(),
        COL_VALUE,
        value.c_str(),
        COL_HELP,
        def.help,  // tooltip text from table
        -1
    );
  }

  GtkWidget *tree_view_widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  GtkTreeView *tree_view = GTK_TREE_VIEW(tree_view_widget);
  g_object_unref(store);

  // Enable tooltips from the hidden help column
  gtk_tree_view_set_tooltip_column(tree_view, COL_HELP);

  auto *ctx = g_new(edit_ctx_t, 1);
  ctx->self = this;
  ctx->tree_view = tree_view;

  auto add_col = [&](const char *title, int col, bool editable, bool expand) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    if (editable) {
      g_object_set(renderer, "editable", TRUE, NULL);
      g_signal_connect_data(
          renderer,
          "edited",
          G_CALLBACK(onValueEdited),
          ctx,
          (GClosureNotify)g_free,
          (GConnectFlags)0
      );
      ctx = nullptr;
    }
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes(title, renderer, "text", col, NULL);
    gtk_tree_view_column_set_expand(column, expand);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(tree_view, column);
  };

  add_col("Key", COL_KEY, false, true);
  add_col("Value", COL_VALUE, true, true);
  add_col("Type", COL_TYPE, false, false);

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
