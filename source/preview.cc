// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include <geanyplugin.h>
#undef geany

#define TRACE_LOCATION() (std::cout << "File: " << __FILE__ << ", Line: " << __LINE__ << '\n')

gboolean preview_init(GeanyPlugin *plugin, gpointer) {
  return TRUE;
}

void preview_cleanup(GeanyPlugin *plugin, gpointer) {}

extern "C" G_MODULE_EXPORT void geany_load_module(GeanyPlugin *plugin) {
  plugin->info->name = "Preview";
  plugin->info->description =
      "Preview pane for Markdown and other lightweight markup languages";
  plugin->info->version = "0.2";
  plugin->info->author = "xiota";

  plugin->funcs->init = preview_init;
  plugin->funcs->cleanup = preview_cleanup;
  plugin->funcs->configure = nullptr;
  plugin->funcs->callbacks = nullptr;
  plugin->funcs->help = nullptr;

  GEANY_PLUGIN_REGISTER(plugin, 225);
}
