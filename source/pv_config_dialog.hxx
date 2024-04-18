/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <filesystem>
#include <locale>

#include "geanyplugin.h"
#include "pv_settings.hxx"

GtkWidget * getConfigDialog(GtkDialog * dialog);

void saveConfig(GtkWidget * self, GtkWidget * dialog);

void reloadConfig(GtkWidget * self, GtkWidget * dialog);

void resetConfig(GtkWidget * self, GtkWidget * dialog);

void editConfig(GtkWidget * self, GtkWidget * dialog);

void openConfigFolder(GtkWidget * self, GtkWidget * dialog);
