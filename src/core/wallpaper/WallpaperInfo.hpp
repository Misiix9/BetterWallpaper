#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace bwp::wallpaper {
enum class WallpaperType {
  StaticImage,
  AnimatedImage,
  Video,
  WEScene,
  WEVideo,
  WEWeb,
  Unknown
};
enum class ScalingMode { Fill, Fit, Stretch, Center, Tile, Zoom };
struct WallpaperInfo {
  std::string id;
  std::string path;
  std::string title;
  std::string source;
  WallpaperType type = WallpaperType::StaticImage;
  std::vector<std::string> tags;
  bool favorite = false;
  int rating = 0;
  int play_count = 0;
  long long added = 0;
  long long last_used = 0;
  bool isScanning = false;
  bool isAutoTagged = false;
  uint64_t workshop_id = 0;
  uint64_t size_bytes = 0;
  std::string blurhash;
  struct Settings {
    int fps = -1; // -1 = use global default
    bool muted = false;
    int volume = -1; // -1 = use global default
    double playback_speed = 1.0;
    ScalingMode scaling = ScalingMode::Fill;
    int noAudioProcessing = -1; // -1 = use global, 0 = off, 1 = on
    int disableMouse = -1;      // -1 = use global, 0 = off, 1 = on
    int noAutomute = -1;        // -1 = use global, 0 = off, 1 = on
  } settings;
};
} // namespace bwp::wallpaper
