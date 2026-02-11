#pragma once
#include <string>
#include <vector>
#include <cstdint>
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
    int fps = 0;  
    bool muted = false;
    int volume = 100;
    double playback_speed = 1.0;
    ScalingMode scaling = ScalingMode::Fill; 
  } settings;
};
}  
