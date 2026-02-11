#pragma once
#include "MonitorInfo.hpp"
#include <memory>
#include <wayland-client.h>
namespace bwp::monitor {
class WaylandMonitor {
public:
  WaylandMonitor(uint32_t id, wl_output *output);
  ~WaylandMonitor();
  const MonitorInfo &getInfo() const { return m_info; }
  wl_output *getOutput() const { return m_output; }
  static void handle_geometry(void *data, wl_output *wl_output, int32_t x,
                              int32_t y, int32_t physical_width,
                              int32_t physical_height, int32_t subpixel,
                              const char *make, const char *model,
                              int32_t transform);
  static void handle_mode(void *data, wl_output *wl_output, uint32_t flags,
                          int32_t width, int32_t height, int32_t refresh);
  static void handle_done(void *data, wl_output *wl_output);
  static void handle_scale(void *data, wl_output *wl_output, int32_t factor);
  static void handle_name(void *data, wl_output *wl_output, const char *name);
  static void handle_description(void *data, wl_output *wl_output,
                                 const char *description);
private:
  uint32_t m_id;
  wl_output *m_output;
  MonitorInfo m_info;
  bool m_ready = false;
};
}  
