// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifndef NDEBUG
#  include <glib.h>
#  define TRACE_LOCATION()                                                 \
    do {                                                                   \
      g_printerr("%s @ %s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    } while (0)
#else
#  define TRACE_LOCATION() \
    do {                   \
    } while (0)
#endif
