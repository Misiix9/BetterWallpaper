#pragma once
#include "../config/ConfigManager.hpp"

namespace bwp::wallpaper {

struct TransitionPolicy {
  bool enableTransitions = true;
  int overlapDelayMs = 800; // Short overlap window to allow external surfaces to swap
};

TransitionPolicy computeTransitionPolicy(const bwp::config::ConfigManager &conf);

struct TransitionPlan {
  bool useWindowTransition = true;
  int stopOldRendererDelayMs = 800;
};

TransitionPlan makeTransitionPlan(const TransitionPolicy &policy);

} // namespace bwp::wallpaper
