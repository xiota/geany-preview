/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-FileCopyrightText: Copyright 2013 Matthew <mbrush@codebrainz.ca>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <vector>

#include "tkui_addons.hxx"

typedef struct FmtProcess FmtProcess;

FmtProcess * fmt_process_open(
    const std::string & work_dir, const std::vector<std::string> & argv_str
);
int fmt_process_close(FmtProcess * proc);
bool fmt_process_run(FmtProcess * proc, const std::string & str_in, std::string & str_out);
