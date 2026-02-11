#include "WaylandMonitor.hpp"
#include "../utils/Logger.hpp"
#include <cstring>
namespace bwp::monitor {
static const struct wl_output_listener output_listener = {
    .geometry = WaylandMonitor::handle_geometry,
    .mode = WaylandMonitor::handle_mode,
    .done = WaylandMonitor::handle_done,
    .scale = WaylandMonitor::handle_scale,
#ifdef WL_OUTPUT_NAME_SINCE_VERSION
    .name = WaylandMonitor::handle_name,
    .description = WaylandMonitor::handle_description,
#endif
};
WaylandMonitor::WaylandMonitor(uint32_t id, wl_output *output)
    : m_id(id), m_output(output) {
  m_info.id = id;
  wl_output_add_listener(output, &output_listener, this);
}
WaylandMonitor::~WaylandMonitor() {
  if (m_output) {
    wl_output_destroy(m_output);
  }
}
void WaylandMonitor::handle_geometry(void *data, wl_output *, int32_t x,
                                     int32_t y, int32_t, int32_t, int32_t,
                                     const char *make, const char *model,
                                     int32_t) {
  auto *self = static_cast<WaylandMonitor *>(data);
  self->m_info.x = x;
  self->m_info.y = y;
  if (make && model) {
    self->m_info.description = std::string(make) + " " + model;
  }
}
void WaylandMonitor::handle_mode(void *data, wl_output *, uint32_t flags,
                                 int32_t width, int32_t height,
                                 int32_t refresh) {
  auto *self = static_cast<WaylandMonitor *>(data);
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    self->m_info.width = width;
    self->m_info.height = height;
    self->m_info.refresh_rate = refresh;
  }
}
void WaylandMonitor::handle_done(void *data, wl_output *) {
  auto *self = static_cast<WaylandMonitor *>(data);
  self->m_ready = true;
  LOG_DEBUG("Monitor " + std::to_string(self->m_id) + " (" + self->m_info.name +
            ") ready");
}
void WaylandMonitor::handle_scale(void *data, wl_output *, int32_t factor) {
  auto *self = static_cast<WaylandMonitor *>(data);
  self->m_info.scale = static_cast<double>(factor);
}
void WaylandMonitor::handle_name(void *data, wl_output *, const char *name) {
  auto *self = static_cast<WaylandMonitor *>(data);
  if (name)
    self->m_info.name = name;
}
void WaylandMonitor::handle_description(void *data, wl_output *,
                                        const char *description) {
  auto *self = static_cast<WaylandMonitor *>(data);
  if (description)
    self->m_info.description = description;
}
}  
