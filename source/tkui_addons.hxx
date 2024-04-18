/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <locale>

#include "geanyplugin.h"

extern GeanyPlugin *geany_plugin;
extern GeanyData *geany_data;

#define DEBUG printf("%s\n\t%d\n\t%s\n\n", __FILE__, __LINE__, __FUNCTION__);

#define GFREE(_z_) \
  do {             \
    g_free(_z_);   \
    _z_ = nullptr; \
  } while (0)

#define GERROR_FREE(_z_) \
  do {                   \
    g_error_free(_z_);   \
    _z_ = nullptr;       \
  } while (0)

#define GEANY_PSC(sig, cb)                                                  \
  plugin_signal_connect(geany_plugin, nullptr, (sig), true, G_CALLBACK(cb), \
                        nullptr)

#if GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 58
#define G_SOURCE_FUNC(f) ((GSourceFunc)(void (*)(void))(f))
#endif  // G_SOURCE_FUNC

// g_clear_signal_handler was added in glib 2.62
#if GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 62
#include "gobject/gsignal.h"
#define g_clear_signal_handler(handler, instance)      \
  do {                                                 \
    if (handler != nullptr && *handler != 0) {         \
      g_signal_handler_disconnect(instance, *handler); \
      *handler = 0;                                    \
    }                                                  \
  } while (0)
#endif  // g_clear_signal_handler
