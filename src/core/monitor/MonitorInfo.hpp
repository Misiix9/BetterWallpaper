#pragma once
#include <cstdint>
#include <string>
namespace bwp::monitor {
struct MonitorInfo {
  std::string name;         
  std::string description;  
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;
  int32_t refresh_rate = 60000;  
  double scale = 1.0;
  uint32_t id = 0;  
  bool enabled = true;
  bool primary = false;
  int32_t logicalWidth() const { return static_cast<int32_t>(width / scale); }
  int32_t logicalHeight() const { return static_cast<int32_t>(height / scale); }
};
}  
