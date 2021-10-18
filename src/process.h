/* -*- C++ -*-
 * process.h
 *
 * Copyright 2013 Matthew <mbrush@codebrainz.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef FMT_PROCESS_H
#define FMT_PROCESS_H

#include "plugin.h"

G_BEGIN_DECLS

typedef struct FmtProcess FmtProcess;

FmtProcess *fmt_process_open(const char *work_dir, const char *const *argv);
int fmt_process_close(FmtProcess *proc);
bool fmt_process_run(FmtProcess *proc, const char *str_in, size_t in_len,
                     GString *str_out);

G_END_DECLS

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

#endif  // FMT_PROCESS_H
