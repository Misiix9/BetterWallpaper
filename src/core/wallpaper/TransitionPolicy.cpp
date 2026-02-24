#include "TransitionPolicy.hpp"

namespace bwp::wallpaper {

TransitionPolicy computeTransitionPolicy(const bwp::config::ConfigManager &conf) {
  TransitionPolicy policy;
  policy.enableTransitions = conf.get<bool>("transitions.enabled", true);
  // Shorter overlap per user choice (0.8s). Disable overlap when transitions are off.
  policy.overlapDelayMs = policy.enableTransitions ? 800 : 0;
  return policy;
}

TransitionPlan makeTransitionPlan(const TransitionPolicy &policy) {
  TransitionPlan plan;
  plan.useWindowTransition = policy.enableTransitions;
  plan.stopOldRendererDelayMs = policy.overlapDelayMs;
  return plan;
}

} // namespace bwp::wallpaper
