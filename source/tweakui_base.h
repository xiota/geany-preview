// SPDX-FileCopyrightText: Copyright 2026 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * SECTION:tweakui_base
 * @title: TweakUiBase
 * @short_description: CRTP base class for tweakui modules
 *
 * TweakUiBase is a small CRTP helper that gives each tweakui module:
 *
 *   • A per-class singleton instance() method
 *   • Automatic registration into tweakui_registry()
 *   • A header-only implementation with no .cc file
 *
 * Registration is performed by a static object inside the CRTP base.
 * Because this object lives in a template, each module must explicitly
 * instantiate its specialization to ensure the registrar is emitted:
 *
 * |[
 *   template class TweakUiBase<TweakUiColorTip>;
 * ]|
 *
 * Example:
 *
 * |[
 *   class TweakUiColorTip : public TweakUiBase<TweakUiColorTip> {
 *   public:
 *     TweakUiColorTip();
 *   };
 *
 *   template class TweakUiBase<TweakUiColorTip>;
 * ]|
 */

#pragma once

#include "tweakui_registry.h"

template <typename Derived>
class TweakUiBase {
 public:
  static Derived &instance() {
    static Derived inst;
    return inst;
  }

 protected:
  TweakUiBase() = default;

 private:
  static AutoRegisterTweakUi<Derived> reg_;
};

template <typename Derived>
AutoRegisterTweakUi<Derived> TweakUiBase<Derived>::reg_{};
