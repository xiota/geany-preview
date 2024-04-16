/*
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

#pragma once

#include <string>
#include <vector>

#include "tkui_addons.h"

typedef struct FmtProcess FmtProcess;

FmtProcess *fmt_process_open(const std::string &work_dir,
                             const std::vector<std::string> &argv_str);
int fmt_process_close(FmtProcess *proc);
bool fmt_process_run(FmtProcess *proc, const std::string &str_in,
                     std::string &str_out);
