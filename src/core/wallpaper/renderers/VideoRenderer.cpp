#include "VideoRenderer.hpp"
#include "../../utils/Logger.hpp"
#include <clocale>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace bwp::wallpaper {

VideoRenderer::VideoRenderer() {
  // Ensure LC_NUMERIC is set to "C" for mpv (it requires C locale for parsing)
  const char *oldLocale = std::setlocale(LC_NUMERIC, nullptr);
  std::string savedLocale = oldLocale ? oldLocale : "C";
  std::setlocale(LC_NUMERIC, "C");

  m_mpv = mpv_create();
  if (!m_mpv) {
    LOG_ERROR("Failed to create MPV instance — libmpv may not be available or "
              "incompatible. Check that libmpv is installed (e.g., "
              "libmpv-dev).");
    // Restore locale
    std::setlocale(LC_NUMERIC, savedLocale.c_str());
    return;
  }

  // Set essential options before initialize
  mpv_set_option_string(m_mpv, "terminal", "no");
  mpv_set_option_string(m_mpv, "msg-level", "all=no");
  mpv_set_option_string(m_mpv, "loop", "yes");
  mpv_set_option_string(m_mpv, "audio", "no");  // Wallpapers typically silent
  mpv_set_option_string(m_mpv, "ytdl", "no");
  mpv_set_option_string(m_mpv, "vo", "libmpv");

  int err = mpv_initialize(m_mpv);
  if (err < 0) {
    LOG_ERROR("Failed to initialize MPV: " + std::string(mpv_error_string(err)));
    mpv_terminate_destroy(m_mpv);
    m_mpv = nullptr;
    std::setlocale(LC_NUMERIC, savedLocale.c_str());
    return;
  }

  // Initialize render context for SW
  mpv_render_param params[] = {
      {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_SW},
      {MPV_RENDER_PARAM_INVALID, nullptr}};

  err = mpv_render_context_create(&m_mpv_ctx, m_mpv, params);
  if (err < 0) {
    LOG_ERROR("Failed to create MPV render context: " +
              std::string(mpv_error_string(err)) +
              " — SW render API may not be supported by this mpv build");
    m_mpv_ctx = nullptr;
    // mpv is still usable for non-render operations, but we need render ctx
  }

  // Restore locale
  std::setlocale(LC_NUMERIC, savedLocale.c_str());

  if (m_mpv && m_mpv_ctx) {
    LOG_INFO("MPV video renderer initialized successfully");
  }
}

VideoRenderer::~VideoRenderer() {
  if (m_mpv_ctx)
    mpv_render_context_free(m_mpv_ctx);
  if (m_mpv)
    mpv_terminate_destroy(m_mpv);
  if (m_buffer)
    delete[] m_buffer;
}

bool VideoRenderer::load(const std::string &path) {
  if (!m_mpv)
    return false;

  const char *cmd[] = {"loadfile", path.c_str(), nullptr};
  mpv_command(m_mpv, cmd);

  play();
  return true;
}

void VideoRenderer::setScalingMode(ScalingMode mode) {
  m_mode = mode;
  // Apply logic if MPV supports geometry options natively,
  // but usually we handle scaling in render()
}

void VideoRenderer::render(cairo_t *cr, int width, int height) {
  if (!m_mpv_ctx)
    return;

  // Resize buffer if needed
  if (width != m_bufWidth || height != m_bufHeight) {
    // Simple reallocation
    if (m_buffer)
      delete[] m_buffer;
    m_bufWidth = width;
    m_bufHeight = height;
    m_buffer = new uint8_t[width * height * 4]; // BGRA
  }

  // Ask MPV to render
  int stride = width * 4;
  // Param size structure check
  int sizes[2] = {width, height};

  mpv_render_param params[] = {
      {MPV_RENDER_PARAM_SW_SIZE, sizes}, // passing ptr to int array {w, h}
      {MPV_RENDER_PARAM_SW_FORMAT, (void *)"bgra"},
      {MPV_RENDER_PARAM_SW_STRIDE, (void *)&stride},
      {MPV_RENDER_PARAM_SW_POINTER, (void *)m_buffer},
      {MPV_RENDER_PARAM_INVALID, nullptr}};

  // Advance MPV state?
  // Ideally we call mpv_render_context_update() check

  // This is blocking render request?

  // Ideally usage is: check update, then render.
  // But for simplicity call render.

  int err = mpv_render_context_render(m_mpv_ctx, params);
  if (err < 0) {
    // NO frame or error
    // But if previous frame exists in buffer, we can just draw it
  }

  // Draw buffer to cairo
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      m_buffer, CAIRO_FORMAT_ARGB32, width, height,
      stride); // ARGB32 accepts BGRA on little endian

  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);

  cairo_surface_destroy(surface);
}

void VideoRenderer::play() {
  if (!m_mpv)
    return;
  int flag = 0;
  mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
  m_paused = false;
}

void VideoRenderer::pause() {
  if (!m_mpv)
    return;
  int flag = 1;
  mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
  m_paused = true;
}

void VideoRenderer::stop() {
  if (!m_mpv)
    return;
  const char *cmd[] = {"stop", nullptr};
  mpv_command(m_mpv, cmd);
  m_paused = true;
}

void VideoRenderer::setVolume(float volume) { // 0.0 - 1.0
  if (!m_mpv)
    return;
  double vol = volume * 100.0;
  mpv_set_property(m_mpv, "volume", MPV_FORMAT_DOUBLE, &vol);
}

void VideoRenderer::setPlaybackSpeed(float speed) {
  if (!m_mpv)
    return;
  double spd = speed;
  mpv_set_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &spd);
}

} // namespace bwp::wallpaper
