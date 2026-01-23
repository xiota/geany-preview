// SPDX-FileCopyrightText: Copyright 2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * SECTION:tweakui_registry
 * @title: TweakUi Registry
 * @short_description: Header-only registry for tweakui initialization
 *
 * The tweakui registry stores initialization callbacks inserted by
 * TweakUiRegistry. Each tweakui module registers a callback that
 * constructs its singleton during plugin startup.
 *
 * This avoids static initialization ordering issues and keeps all tweakui
 * modules header-only and self-contained.
 *
 * Example:
 *
 * |[
 *   for (auto &init : tweakui_registry())
 *     init();
 * ]|
 */

#pragma once

#include <functional>
#include <vector>

// Registry of initialization callbacks
inline std::vector<std::function<void()>> &tweakui_registry() {
  static std::vector<std::function<void()>> registry;
  return registry;
}

// Auto-registration helper
template <typename T>
struct TweakUiRegistry {
  TweakUiRegistry() {
    tweakui_registry().push_back([] { T::instance(); });
  }
};
