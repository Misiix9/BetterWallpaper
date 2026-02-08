#include <gtest/gtest.h>
#include "core/scheduler/Scheduler.hpp"

using bwp::core::Scheduler;
using bwp::core::ScheduleEntry;

// ──────────────────────────────────────────────────────────
//  Scheduler — Singleton
// ──────────────────────────────────────────────────────────

TEST(Scheduler, SingletonReturnsConsistentRef) {
  auto &a = Scheduler::getInstance();
  auto &b = Scheduler::getInstance();
  EXPECT_EQ(&a, &b);
}

// ──────────────────────────────────────────────────────────
//  Scheduler — Add/Remove/Get schedules
// ──────────────────────────────────────────────────────────

TEST(Scheduler, AddScheduleAndRetrieve) {
  auto &sched = Scheduler::getInstance();

  ScheduleEntry entry;
  entry.id = "test_sched_001";
  entry.name = "Morning";
  entry.profileId = "profile_a";
  entry.startTime = 480; // 08:00
  entry.endTime = 720;   // 12:00
  entry.enabled = true;

  sched.addSchedule(entry);

  auto schedules = sched.getSchedules();
  bool found = false;
  for (const auto &s : schedules) {
    if (s.id == "test_sched_001") {
      found = true;
      EXPECT_EQ(s.name, "Morning");
      EXPECT_EQ(s.profileId, "profile_a");
      EXPECT_EQ(s.startTime, 480);
      EXPECT_EQ(s.endTime, 720);
      EXPECT_TRUE(s.enabled);
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST(Scheduler, RemoveSchedule) {
  auto &sched = Scheduler::getInstance();

  ScheduleEntry entry;
  entry.id = "test_sched_remove";
  entry.name = "Removable";
  entry.profileId = "profile_b";
  entry.startTime = 0;
  entry.endTime = 0;
  entry.enabled = false;

  sched.addSchedule(entry);
  sched.removeSchedule("test_sched_remove");

  auto schedules = sched.getSchedules();
  bool found = false;
  for (const auto &s : schedules) {
    if (s.id == "test_sched_remove") {
      found = true;
      break;
    }
  }
  EXPECT_FALSE(found);
}

TEST(Scheduler, RemoveNonExistentIsNoOp) {
  auto &sched = Scheduler::getInstance();
  EXPECT_NO_THROW(sched.removeSchedule("__does_not_exist__"));
}

// ──────────────────────────────────────────────────────────
//  Scheduler — checkSchedules doesn't crash
// ──────────────────────────────────────────────────────────

TEST(Scheduler, CheckSchedulesDoesNotCrash) {
  auto &sched = Scheduler::getInstance();
  // Just verify it doesn't segfault (especially with our localtime_r fix)
  EXPECT_NO_THROW(sched.checkSchedules());
}

// ──────────────────────────────────────────────────────────
//  Cleanup
// ──────────────────────────────────────────────────────────

TEST(Scheduler, Cleanup) {
  auto &sched = Scheduler::getInstance();
  sched.removeSchedule("test_sched_001");
}
