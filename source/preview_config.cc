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
struct EditContext {
  PreviewConfig *self;
  GtkTreeView *tree_view;
};

// Context for search/filter callbacks
struct SearchContext {
  std::string search_text;
  GtkTreeModelFilter *filter_model = nullptr;
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
  auto *ctx = static_cast<EditContext *>(user_data);
  GtkTreeModel *model = gtk_tree_view_get_model(ctx->tree_view);

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_from_string(model, &iter, path_str)) {
    return;
  }

  // Resolve to underlying store if model is a filter
  GtkTreeModel *store_model = model;
  GtkTreeIter store_iter;
  if (GTK_IS_TREE_MODEL_FILTER(model)) {
    store_model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
    gtk_tree_model_filter_convert_iter_to_child_iter(
        GTK_TREE_MODEL_FILTER(model), &store_iter, &iter
    );
  } else {
    store_iter = iter;
  }

  gboolean active;
  gchar *key_c;
  gtk_tree_model_get(store_model, &store_iter, COL_VALUE_BOOL, &active, COL_KEY, &key_c, -1);

  ctx->self->set<bool>(key_c, !active);
  gtk_list_store_set(GTK_LIST_STORE(store_model), &store_iter, COL_VALUE_BOOL, !active, -1);

  g_free(key_c);
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
  auto *ctx = static_cast<EditContext *>(user_data);
  GtkTreeModel *model = gtk_tree_view_get_model(ctx->tree_view);

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_from_string(model, &iter, path_str)) {
    return;
  }

  // Resolve to underlying store if model is a filter
  GtkTreeModel *store_model = model;
  GtkTreeIter store_iter;
  if (GTK_IS_TREE_MODEL_FILTER(model)) {
    store_model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));
    gtk_tree_model_filter_convert_iter_to_child_iter(
        GTK_TREE_MODEL_FILTER(model), &store_iter, &iter
    );
  } else {
    store_iter = iter;
  }

  gchar *key_c;
  gchar *type_c;
  gtk_tree_model_get(store_model, &store_iter, COL_KEY, &key_c, COL_TYPE, &type_c, -1);

  std::string key = key_c;
  std::string type = type_c;
  g_free(key_c);
  g_free(type_c);

  if (type == "int") {
    try {
      ctx->self->set<int>(key, std::stoi(new_text));
      gtk_list_store_set(GTK_LIST_STORE(store_model), &store_iter, COL_VALUE_STR, new_text, -1);
    } catch (...) {
      g_warning("Invalid integer: %s", new_text);
    }
  } else if (type == "string") {
    ctx->self->set<std::string>(key, new_text);
    gtk_list_store_set(GTK_LIST_STORE(store_model), &store_iter, COL_VALUE_STR, new_text, -1);
  } else if (type == "vector<int>") {
    ctx->self->set<std::vector<int>>(key, parseIntsFlexible(new_text));
    gtk_list_store_set(GTK_LIST_STORE(store_model), &store_iter, COL_VALUE_STR, new_text, -1);
  } else if (type == "vector<string>") {
    ctx->self->set<std::vector<std::string>>(key, splitFlexible(new_text));
    gtk_list_store_set(GTK_LIST_STORE(store_model), &store_iter, COL_VALUE_STR, new_text, -1);
  }
}

static gboolean filterVisibleFunc(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
  auto *ctx = static_cast<SearchContext *>(data);

  if (ctx->search_text.empty()) {
    return true;  // show all
  }

  gchar *key_c = nullptr;
  gchar *help_c = nullptr;
  gtk_tree_model_get(model, iter, COL_KEY, &key_c, COL_HELP, &help_c, -1);

  std::string key = key_c ? key_c : "";
  std::string help = help_c ? help_c : "";
  g_free(key_c);
  g_free(help_c);

  auto toLower = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return std::tolower(c);
    });
    return s;
  };

  std::string search = toLower(ctx->search_text);
  key = toLower(key);
  help = toLower(help);

  return (key.find(search) != std::string::npos) || (help.find(search) != std::string::npos);
}

static void onSearchEntryChanged(GtkEditable *editable, gpointer user_data) {
  auto *ctx = static_cast<SearchContext *>(user_data);
  const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
  ctx->search_text = text ? text : "";
  gtk_tree_model_filter_refilter(ctx->filter_model);
}

}  // namespace

PreviewConfig::PreviewConfig(
    const std::filesystem::path &config_path,
    std::string_view config_file
)
    : config_path_(std::filesystem::weakly_canonical(config_path)), config_file_(config_file) {
  auto full_path = config_path_ / config_file_;
  std::filesystem::create_directories(config_path_);

  // Populate settings_ and help_texts_ from the master table
  for (const auto &def : setting_defs_) {
    settings_[def.key] = def.default_value;
    help_texts_[def.key] = def.help;
  }
}

bool PreviewConfig::load() {
  auto full_path = config_path_ / config_file_;
  if (!std::filesystem::exists(full_path)) {
    return false;
  }

  try {
    auto tbl = toml::parse_file(full_path.string());
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

  auto full_path = config_path_ / config_file_;
  try {
    std::ofstream out(full_path, std::ios::trunc);
    out << root;
    return true;
  } catch (...) {
    g_warning("Failed to save config to %s", full_path.string().c_str());
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

// Builds and returns a populated GtkListStore from settings_
GtkListStore *PreviewConfig::createConfigModel() {
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

  for (const auto &def : setting_defs_) {
    auto it = settings_.find(def.key);
    if (it == settings_.end()) {
      continue;
    }

    const auto &val = it->second;
    std::string type, value_str;
    gboolean value_bool = false;

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

  return store;
}

// Creates the GtkTreeView and its columns (no signals yet)
GtkTreeView *PreviewConfig::createConfigTreeView(GtkListStore *store) {
  GtkWidget *tree_view_widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  GtkTreeView *tree_view = GTK_TREE_VIEW(tree_view_widget);

  // Tooltips from hidden help column
  gtk_tree_view_set_tooltip_column(tree_view, COL_HELP);

  // Column: Key
  {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes("Key", renderer, "text", COL_KEY, NULL);
    gtk_tree_view_column_set_expand(column, true);
    gtk_tree_view_column_set_resizable(column, true);
    gtk_tree_view_append_column(tree_view, column);
  }

  // Column: Value (toggle + text)
  {
    GtkTreeViewColumn *value_column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(value_column, "Value");

    // Toggle renderer for bools
    GtkCellRenderer *toggle_renderer = gtk_cell_renderer_toggle_new();
    g_object_set(toggle_renderer, "xalign", 0.0f, NULL);
    gtk_tree_view_column_pack_start(value_column, toggle_renderer, false);
    gtk_tree_view_column_add_attribute(value_column, toggle_renderer, "active", COL_VALUE_BOOL);
    gtk_tree_view_column_add_attribute(
        value_column, toggle_renderer, "visible", COL_SHOW_TOGGLE
    );

    // Text renderer for others
    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
    g_object_set(text_renderer, "editable", true, NULL);
    gtk_tree_view_column_pack_start(value_column, text_renderer, true);
    gtk_tree_view_column_add_attribute(value_column, text_renderer, "text", COL_VALUE_STR);
    gtk_tree_view_column_add_attribute(value_column, text_renderer, "visible", COL_SHOW_TEXT);

    gtk_tree_view_column_set_expand(value_column, true);
    gtk_tree_view_column_set_resizable(value_column, true);
    gtk_tree_view_append_column(tree_view, value_column);
  }

  // Column: Type
  {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes("Type", renderer, "text", COL_TYPE, NULL);
    gtk_tree_view_column_set_resizable(column, true);
    gtk_tree_view_append_column(tree_view, column);
  }

  return tree_view;
}

// Connects signals for editing and dialog responses
void PreviewConfig::connectConfigSignals(GtkTreeView *tree_view, GtkDialog *dialog) {
  auto *ctx = g_new(EditContext, 1);
  ctx->self = this;
  ctx->tree_view = tree_view;

  // Find the value column (second column in our setup)
  GList *columns = gtk_tree_view_get_columns(tree_view);
  GtkTreeViewColumn *value_column = GTK_TREE_VIEW_COLUMN(g_list_nth_data(columns, 1));
  g_list_free(columns);

  // Get renderers from value column
  GList *renderers = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(value_column));
  GtkCellRenderer *toggle_renderer = GTK_CELL_RENDERER(g_list_nth_data(renderers, 0));
  GtkCellRenderer *text_renderer = GTK_CELL_RENDERER(g_list_nth_data(renderers, 1));
  g_list_free(renderers);

  g_signal_connect(toggle_renderer, "toggled", G_CALLBACK(onBoolToggled), ctx);
  g_signal_connect_data(
      text_renderer,
      "edited",
      G_CALLBACK(onValueEdited),
      ctx,
      (GClosureNotify)g_free,
      (GConnectFlags)0
  );

  g_signal_connect(
      dialog,
      "response",
      G_CALLBACK(+[](GtkDialog *d, gint id, gpointer ud) {
        static_cast<PreviewConfig *>(ud)->onDialogResponse(d, id);
      }),
      this
  );
}

GtkWidget *PreviewConfig::buildConfigWidget(GtkDialog *dialog) {
  GtkListStore *store = createConfigModel();

  // Allocate search context
  auto *ctx = g_new0(SearchContext, 1);

  // Wrap store in filter model
  ctx->filter_model =
      GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(store), nullptr));
  g_object_unref(store);  // filter holds a ref

  gtk_tree_model_filter_set_visible_func(ctx->filter_model, filterVisibleFunc, ctx, nullptr);

  GtkTreeView *tree_view = createConfigTreeView(GTK_LIST_STORE(ctx->filter_model));
  connectConfigSignals(tree_view, dialog);

  // Scrolled window for tree view
  GtkWidget *scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(tree_view));
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC
  );
  gtk_widget_set_size_request(scrolled_window, 500, 250);

  // Search entry at bottom
  GtkWidget *search_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search...");
  g_signal_connect(search_entry, "changed", G_CALLBACK(onSearchEntryChanged), ctx);

  // Pack into vertical box
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
  gtk_box_pack_start(GTK_BOX(vbox), search_entry, false, false, 0);

  // Free context when dialog is destroyed
  g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), ctx);

  return vbox;
}
