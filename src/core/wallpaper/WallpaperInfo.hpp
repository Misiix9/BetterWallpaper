#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace bwp::wallpaper {

enum class WallpaperType {
  Unknown,
  StaticImage,   // PNG, JPG, etc.
  AnimatedImage, // GIF, APNG (handled by GdkPixbuf/Cairo usually, or MPV)
  Video,         // MP4, WEBM
  WEScene,       // Wallpaper Engine Scene
  WEVideo,       // Wallpaper Engine Video
  WEWeb          // Wallpaper Engine Web (Blocked)
};

enum class ScalingMode {
  Stretch, // Fill screen, distort
  Fill,    // Crop to fill, maintain aspect
  Fit,     // Letterbox
  Center,  // No scaling
  Tile,    // Repeat
  Zoom     // Custom zoom
};

struct WallpaperInfo {
  std::string id;
  std::string path;
  WallpaperType type = WallpaperType::Unknown;
  std::string format;
  int32_t width = 0;
  int32_t height = 0;
  int64_t size_bytes = 0;
  std::string hash;
  std::string source = "local"; // local, workshop
  std::optional<uint64_t> workshop_id;
  std::vector<std::string> tags;
  int rating = 0;
  bool favorite = false;
  int play_count = 0;
  std::chrono::system_clock::time_point last_used;
  std::chrono::system_clock::time_point added;
  std::vector<std::string> colors;

  // Settings Overrides
  struct Settings {
    int fps = 0;      // 0 = default/unlimited
    int volume = 100; // 0-100
    bool muted = false;
    float playback_speed = 1.0f;
    ScalingMode scaling = ScalingMode::Fill;
  } settings;
};

} // namespace bwp::wallpaper
