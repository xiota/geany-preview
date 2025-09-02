// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @brief Lightweight struct holding non-owning pointers to core plugin components.
 */

class PreviewPane;
class PreviewConfig;

struct PreviewContext {
  PreviewPane *preview_pane_ = nullptr;
  PreviewConfig *preview_config_ = nullptr;
};
