#pragma once
#include <cstdint>
#include <string>

namespace bwp::monitor {

struct MonitorInfo {
  std::string name;        // e.g., "DP-1"
  std::string description; // e.g., "LG Electronics LG Ultragear 123456"
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;
  int32_t refresh_rate = 60000; // in mHz, e.g. 60000 = 60Hz
  double scale = 1.0;

  // For Wayland identification
  uint32_t id = 0; // Global ID from registry

  bool enabled = true;
  bool primary = false;

  // Helper to get logic resolution
  int32_t logicalWidth() const { return static_cast<int32_t>(width / scale); }
  int32_t logicalHeight() const { return static_cast<int32_t>(height / scale); }
};

} // namespace bwp::monitor
