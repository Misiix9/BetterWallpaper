#include "TransitionEngine.hpp"
#include <iostream>

namespace bwp::transition {

TransitionEngine::TransitionEngine() {}

TransitionEngine::~TransitionEngine() { stop(); }

void TransitionEngine::start(cairo_surface_t *from, cairo_surface_t *to,
                             std::shared_ptr<TransitionEffect> effect,
                             long durationMs, FinishCallback callback) {
  stop(); // constrained cleanup

  if (from) {
    m_from = cairo_surface_reference(from);
  }
  if (to) {
    m_to = cairo_surface_reference(to);
  }

  m_effect = effect;
  m_durationMs = durationMs;
  m_callback = callback;
  m_startTime = std::chrono::steady_clock::now();
  m_active = true;
}

void TransitionEngine::stop() {
  m_active = false;
  if (m_from) {
    cairo_surface_destroy(m_from);
    m_from = nullptr;
  }
  if (m_to) {
    cairo_surface_destroy(m_to);
    m_to = nullptr;
  }
}

bool TransitionEngine::render(cairo_t *cr, int width, int height) {
  if (!m_active)
    return false;

  auto now = std::chrono::steady_clock::now();
  long elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime)
          .count();

  double progress = (double)elapsed / (double)m_durationMs;

  if (progress >= 1.0) {
    // Render final state one last time
    if (m_effect && m_from && m_to) {
      m_effect->render(cr, m_from, m_to, 1.0, width, height, {});
    }

    // Finish
    if (m_callback) {
      m_callback();
    }
    stop();
    return false;
  }

  if (m_effect && m_from && m_to) {
    m_effect->render(cr, m_from, m_to, progress, width, height, {});
  }

  return true;
}

} // namespace bwp::transition
