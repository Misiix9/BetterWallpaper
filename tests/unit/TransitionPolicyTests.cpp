#include <gtest/gtest.h>
#include "core/wallpaper/TransitionPolicy.hpp"
#include "core/config/ConfigManager.hpp"

using bwp::wallpaper::TransitionPolicy;
using bwp::wallpaper::TransitionPlan;
using bwp::wallpaper::computeTransitionPolicy;
using bwp::wallpaper::makeTransitionPlan;
using bwp::config::ConfigManager;

TEST(TransitionPolicy, EnabledUsesWindowEngineWithOverlap) {
  auto &conf = ConfigManager::getInstance();
  conf.set("transitions.enabled", true);
  conf.set("transitions.default_effect", std::string("Fade"));
  conf.set("transitions.duration_ms", 500);

  auto policy = computeTransitionPolicy(conf);
  auto plan = makeTransitionPlan(policy);

  EXPECT_TRUE(policy.enableTransitions);
  EXPECT_TRUE(plan.useWindowTransition);
  EXPECT_GE(policy.overlapDelayMs, 700);
  EXPECT_LE(policy.overlapDelayMs, 900);
  EXPECT_EQ(plan.stopOldRendererDelayMs, policy.overlapDelayMs);
}

TEST(TransitionPolicy, DisabledDisablesTransitionsAndOverlap) {
  auto &conf = ConfigManager::getInstance();
  conf.set("transitions.enabled", false);

  auto policy = computeTransitionPolicy(conf);
  auto plan = makeTransitionPlan(policy);

  EXPECT_FALSE(policy.enableTransitions);
  EXPECT_FALSE(plan.useWindowTransition);
  EXPECT_EQ(policy.overlapDelayMs, 0);
  EXPECT_EQ(plan.stopOldRendererDelayMs, 0);

  // Restore default true for other tests
  conf.set("transitions.enabled", true);
}
