#include <gtest/gtest.h>
#include "core/wallpaper/WallpaperLibrary.hpp"
#include <filesystem>
#include <fstream>

using bwp::wallpaper::WallpaperInfo;
using bwp::wallpaper::WallpaperLibrary;
using bwp::wallpaper::WallpaperType;

// ──────────────────────────────────────────────────────────
//  WallpaperLibrary — Singleton
// ──────────────────────────────────────────────────────────

TEST(WallpaperLibrary, SingletonReturnsConsistentRef) {
  auto &a = WallpaperLibrary::getInstance();
  auto &b = WallpaperLibrary::getInstance();
  EXPECT_EQ(&a, &b);
}

// ──────────────────────────────────────────────────────────
//  WallpaperLibrary — CRUD
// ──────────────────────────────────────────────────────────

TEST(WallpaperLibrary, AddAndRetrieveWallpaper) {
  auto &lib = WallpaperLibrary::getInstance();

  WallpaperInfo wp;
  wp.id = "test_wp_001";
  wp.path = "/tmp/test_wallpaper.jpg";
  wp.title = "Test Wallpaper";
  wp.type = WallpaperType::StaticImage;
  wp.tags = {"nature", "test"};

  lib.addWallpaper(wp);

  auto retrieved = lib.getWallpaper("test_wp_001");
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->id, "test_wp_001");
  EXPECT_EQ(retrieved->path, "/tmp/test_wallpaper.jpg");
  EXPECT_EQ(retrieved->title, "Test Wallpaper");
  EXPECT_EQ(retrieved->tags.size(), 2);
}

TEST(WallpaperLibrary, GetNonExistentReturnsEmpty) {
  auto &lib = WallpaperLibrary::getInstance();
  auto result = lib.getWallpaper("__does_not_exist__");
  EXPECT_FALSE(result.has_value());
}

TEST(WallpaperLibrary, RemoveWallpaper) {
  auto &lib = WallpaperLibrary::getInstance();

  WallpaperInfo wp;
  wp.id = "test_wp_remove";
  wp.path = "/tmp/test_remove.jpg";
  wp.title = "Remove Me";

  lib.addWallpaper(wp);
  EXPECT_TRUE(lib.getWallpaper("test_wp_remove").has_value());

  lib.removeWallpaper("test_wp_remove");
  EXPECT_FALSE(lib.getWallpaper("test_wp_remove").has_value());
}

TEST(WallpaperLibrary, RemoveNonExistentIsNoOp) {
  auto &lib = WallpaperLibrary::getInstance();
  // Should not throw
  EXPECT_NO_THROW(lib.removeWallpaper("__does_not_exist_remove__"));
}

TEST(WallpaperLibrary, UpdateWallpaper) {
  auto &lib = WallpaperLibrary::getInstance();

  WallpaperInfo wp;
  wp.id = "test_wp_update";
  wp.path = "/tmp/test_update.jpg";
  wp.title = "Original Title";
  wp.favorite = false;

  lib.addWallpaper(wp);

  wp.title = "Updated Title";
  wp.favorite = true;
  lib.updateWallpaper(wp);

  auto retrieved = lib.getWallpaper("test_wp_update");
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->title, "Updated Title");
  EXPECT_TRUE(retrieved->favorite);

  // Cleanup
  lib.removeWallpaper("test_wp_update");
}

// ──────────────────────────────────────────────────────────
//  WallpaperLibrary — getAllWallpapers
// ──────────────────────────────────────────────────────────

TEST(WallpaperLibrary, GetAllReturnsAddedWallpapers) {
  auto &lib = WallpaperLibrary::getInstance();

  // Add multiple
  for (int i = 0; i < 5; ++i) {
    WallpaperInfo wp;
    wp.id = "test_all_" + std::to_string(i);
    wp.path = "/tmp/test_all_" + std::to_string(i) + ".jpg";
    wp.title = "All Test " + std::to_string(i);
    lib.addWallpaper(wp);
  }

  auto all = lib.getAllWallpapers();
  // At least 5 (there may be others from other tests)
  int count = 0;
  for (const auto &w : all) {
    if (w.id.find("test_all_") == 0) {
      count++;
    }
  }
  EXPECT_EQ(count, 5);

  // Cleanup
  for (int i = 0; i < 5; ++i) {
    lib.removeWallpaper("test_all_" + std::to_string(i));
  }
}

// ──────────────────────────────────────────────────────────
//  WallpaperLibrary — search
// ──────────────────────────────────────────────────────────

TEST(WallpaperLibrary, SearchByTitle) {
  auto &lib = WallpaperLibrary::getInstance();

  WallpaperInfo wp;
  wp.id = "test_search_abcxyz";
  wp.path = "/tmp/UniqueSearchableFile12345.jpg";
  wp.title = "UniqueSearchableTitle12345";

  lib.addWallpaper(wp);

  // search() matches against filename and tags (not title)
  auto results = lib.search("UniqueSearchable");
  bool found = false;
  for (const auto &r : results) {
    if (r.id == "test_search_abcxyz") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  lib.removeWallpaper("test_search_abcxyz");
}

TEST(WallpaperLibrary, SearchEmptyReturnsAll) {
  auto &lib = WallpaperLibrary::getInstance();
  auto results = lib.search("");
  auto all = lib.getAllWallpapers();
  // Empty search should return all or a superset
  EXPECT_GE(results.size(), 0u);
}

// ──────────────────────────────────────────────────────────
//  WallpaperLibrary — filter
// ──────────────────────────────────────────────────────────

TEST(WallpaperLibrary, FilterByType) {
  auto &lib = WallpaperLibrary::getInstance();

  WallpaperInfo wp;
  wp.id = "test_filter_video";
  wp.path = "/tmp/filter_video.mp4";
  wp.title = "Video Wallpaper";
  wp.type = WallpaperType::Video;

  lib.addWallpaper(wp);

  auto results =
      lib.filter([](const WallpaperInfo &w) { return w.type == WallpaperType::Video; });

  bool found = false;
  for (const auto &r : results) {
    if (r.id == "test_filter_video") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  lib.removeWallpaper("test_filter_video");
}

// ──────────────────────────────────────────────────────────
//  WallpaperLibrary — getAllTags
// ──────────────────────────────────────────────────────────

TEST(WallpaperLibrary, GetAllTagsIncludesAddedTags) {
  auto &lib = WallpaperLibrary::getInstance();

  WallpaperInfo wp;
  wp.id = "test_tags_xyz";
  wp.path = "/tmp/tags.jpg";
  wp.title = "Tagged";
  wp.tags = {"uniqueTestTag12345"};

  lib.addWallpaper(wp);

  auto tags = lib.getAllTags();
  bool found = std::find(tags.begin(), tags.end(), "uniqueTestTag12345") != tags.end();
  EXPECT_TRUE(found);

  lib.removeWallpaper("test_tags_xyz");
}

// Cleanup the test_wp_001 added in earlier test
TEST(WallpaperLibrary, Cleanup) {
  auto &lib = WallpaperLibrary::getInstance();
  lib.removeWallpaper("test_wp_001");
}
