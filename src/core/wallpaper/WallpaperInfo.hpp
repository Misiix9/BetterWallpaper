#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace bwp::wallpaper {

enum class WallpaperType {
  StaticImage,
  AnimatedImage, // GIF
  Video,        // MP4, WEBM
  WEScene,      // Wallpaper Engine Scene
  WEVideo,       // Wallpaper Engine Video
  WEWeb,
  Unknown
};

enum class ScalingMode {
    Fill,
    Fit,
    Stretch,
    Center,
    Tile,
    Zoom
};

struct WallpaperInfo {
  std::string id;
  std::string path;
  std::string title;
  std::string source; // "local", "steam", "workshop"
  WallpaperType type = WallpaperType::StaticImage;

  // Metadata
  std::vector<std::string> tags;
  bool favorite = false;
  int rating = 0; // 0-5
  int play_count = 0;
  long long added = 0; // Timestamp
  long long last_used = 0; // Timestamp
  
  // AI / Analysis Status
  bool isScanning = false;
  bool isAutoTagged = false;
  
  uint64_t workshop_id = 0;
  uint64_t size_bytes = 0;

  // Progressive loading placeholder
  std::string blurhash; // Compact blur placeholder (e.g. "LEHV6nWB2yk8pyoJadR*.7kCMdnj")

  // Settings Override
  struct Settings {
    int fps = 0; // 0 = default
    bool muted = false;
    int volume = 100;
    double playback_speed = 1.0;
    ScalingMode scaling = ScalingMode::Fill; 
  } settings;
};

// Serialization support (if using nlohmann)
// ... usually defined in Library or serialization helper

} // namespace bwp::wallpaper
