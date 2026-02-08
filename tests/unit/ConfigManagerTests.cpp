#include <gtest/gtest.h>
#include "core/config/ConfigManager.hpp"
#include <filesystem>
#include <fstream>

using bwp::config::ConfigManager;

// ──────────────────────────────────────────────────────────
//  ConfigManager — Singleton access
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, GetInstanceReturnsSameRef) {
  auto &a = ConfigManager::getInstance();
  auto &b = ConfigManager::getInstance();
  EXPECT_EQ(&a, &b);
}

// ──────────────────────────────────────────────────────────
//  ConfigManager — get/set round-trips
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, SetAndGetString) {
  auto &conf = ConfigManager::getInstance();
  conf.set("test.string_val", std::string("hello_test"));
  auto val = conf.get<std::string>("test.string_val");
  EXPECT_EQ(val, "hello_test");
}

TEST(ConfigManager, SetAndGetInt) {
  auto &conf = ConfigManager::getInstance();
  conf.set("test.int_val", 42);
  auto val = conf.get<int>("test.int_val");
  EXPECT_EQ(val, 42);
}

TEST(ConfigManager, SetAndGetBool) {
  auto &conf = ConfigManager::getInstance();
  conf.set("test.bool_val", true);
  EXPECT_TRUE(conf.get<bool>("test.bool_val"));
  conf.set("test.bool_val", false);
  EXPECT_FALSE(conf.get<bool>("test.bool_val"));
}

TEST(ConfigManager, SetAndGetDouble) {
  auto &conf = ConfigManager::getInstance();
  conf.set("test.double_val", 3.14);
  auto val = conf.get<double>("test.double_val");
  EXPECT_DOUBLE_EQ(val, 3.14);
}

TEST(ConfigManager, SetAndGetVector) {
  auto &conf = ConfigManager::getInstance();
  std::vector<std::string> tags = {"a", "b", "c"};
  conf.set("test.vec_val", tags);
  auto val = conf.get<std::vector<std::string>>("test.vec_val");
  EXPECT_EQ(val, tags);
}

// ──────────────────────────────────────────────────────────
//  ConfigManager — default values
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, GetWithDefaultReturnsDefault) {
  auto &conf = ConfigManager::getInstance();
  // Access a key that doesn't exist with an explicit default
  auto val = conf.get<std::string>("nonexistent.key999", "fallback");
  EXPECT_EQ(val, "fallback");
}

TEST(ConfigManager, GetMissingKeyReturnsDefaultConstructed) {
  auto &conf = ConfigManager::getInstance();
  // Without explicit default, should return default-constructed T
  auto val = conf.get<int>("nonexistent.key888");
  EXPECT_EQ(val, 0);
}

// ──────────────────────────────────────────────────────────
//  ConfigManager — overwrite behavior
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, OverwriteExistingKey) {
  auto &conf = ConfigManager::getInstance();
  conf.set("test.overwrite", 1);
  EXPECT_EQ(conf.get<int>("test.overwrite"), 1);
  conf.set("test.overwrite", 2);
  EXPECT_EQ(conf.get<int>("test.overwrite"), 2);
}

// ──────────────────────────────────────────────────────────
//  ConfigManager — load/save cycle
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, LoadReturnsTrue) {
  auto &conf = ConfigManager::getInstance();
  // load() should succeed (creates default config if needed)
  EXPECT_TRUE(conf.load());
}

TEST(ConfigManager, SaveReturnsTrue) {
  auto &conf = ConfigManager::getInstance();
  conf.load();
  EXPECT_TRUE(conf.save());
}

// ──────────────────────────────────────────────────────────
//  ConfigManager — resetToDefaults
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, ResetToDefaultsRestoresValues) {
  auto &conf = ConfigManager::getInstance();

  // Set a custom value
  conf.set("test.custom_reset", std::string("custom"));
  EXPECT_EQ(conf.get<std::string>("test.custom_reset"), "custom");

  // Reset — the custom key should no longer exist (or default)
  conf.resetToDefaults();
  auto val = conf.get<std::string>("test.custom_reset");
  EXPECT_TRUE(val.empty()); // Should have been wiped or reverted to empty default
}

// ──────────────────────────────────────────────────────────
//  ConfigManager — change callback
// ──────────────────────────────────────────────────────────

TEST(ConfigManager, ChangeCallbackIsFired) {
  auto &conf = ConfigManager::getInstance();

  std::string changedKey;
  conf.startWatching([&changedKey](const std::string &key,
                                   const nlohmann::json &) {
    changedKey = key;
  });

  conf.set("test.callback_key", 99);
  EXPECT_EQ(changedKey, "test.callback_key");

  // Clean up — set a null callback
  conf.startWatching(nullptr);
}
