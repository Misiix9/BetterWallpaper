#include "TransitionEngine.hpp"
#include "../utils/Logger.hpp"
#include <cairo.h>
namespace bwp::transition {
TransitionEngine::TransitionEngine() {
  m_lastProgressUpdate = std::chrono::steady_clock::now();
}
TransitionEngine::~TransitionEngine() {
  stop();
  clearPreload();
}
void TransitionEngine::start(cairo_surface_t *from, cairo_surface_t *to,
                             std::shared_ptr<TransitionEffect> effect,
                             long durationMs, const std::string &easingName,
                             FinishCallback callback) {
  startWithEasing(from, to, effect, durationMs, Easing::getByName(easingName),
                  callback);
}
void TransitionEngine::startWithEasing(cairo_surface_t *from,
                                       cairo_surface_t *to,
                                       std::shared_ptr<TransitionEffect> effect,
                                       long durationMs,
                                       Easing::EasingFunc easingFunc,
                                       FinishCallback callback) {
  stop();  
  m_liveToRender = nullptr;
  m_liveToWidth = m_liveToHeight = 0;
  if (m_preloaded && to == nullptr) {
    to = m_preloaded;
    m_preloaded = nullptr;
  }
  if (from) {
    m_from = cairo_surface_reference(from);
  }
  if (to) {
    m_to = cairo_surface_reference(to);
  }
  m_effect = effect;
  m_durationMs = std::max(1L, durationMs);  
  m_easingFunc = easingFunc ? easingFunc : Easing::easeInOutQuad;
  m_callback = callback;
  m_startTime = std::chrono::steady_clock::now();
  m_cachedProgress = 0.0;
  m_cachedEasedProgress = 0.0;
  m_active = true;
  LOG_DEBUG("Transition started: duration=" + std::to_string(durationMs) +
            "ms");
}
void TransitionEngine::startWithLiveTo(cairo_surface_t *from,
                                       LiveToRenderFunc liveToRender,
                                       int width, int height,
                                       std::shared_ptr<TransitionEffect> effect,
                                       long durationMs,
                                       const std::string &easingName,
                                       FinishCallback callback) {
  stop();
  m_to = nullptr;
  if (from) {
    m_from = cairo_surface_reference(from);
  }
  m_liveToRender = std::move(liveToRender);
  m_liveToWidth = width;
  m_liveToHeight = height;
  m_effect = effect;
  m_durationMs = std::max(1L, durationMs);
  Easing::EasingFunc ef = Easing::getByName(easingName);
  m_easingFunc = ef ? ef : Easing::easeInOutQuad;
  m_callback = callback;
  m_startTime = std::chrono::steady_clock::now();
  m_cachedProgress = 0.0;
  m_cachedEasedProgress = 0.0;
  m_active = true;
  LOG_DEBUG("Transition started (live to): duration=" + std::to_string(durationMs) + "ms");
}
void TransitionEngine::preload(cairo_surface_t *nextSurface) {
  clearPreload();
  if (nextSurface) {
    m_preloaded = cairo_surface_reference(nextSurface);
    LOG_DEBUG("Preloaded next wallpaper surface for transition");
  }
}
void TransitionEngine::clearPreload() {
  if (m_preloaded) {
    cairo_surface_destroy(m_preloaded);
    m_preloaded = nullptr;
  }
}
void TransitionEngine::stop() {
  m_active = false;
  m_liveToRender = nullptr;
  m_liveToWidth = m_liveToHeight = 0;
  if (m_from) {
    cairo_surface_destroy(m_from);
    m_from = nullptr;
  }
  if (m_to) {
    cairo_surface_destroy(m_to);
    m_to = nullptr;
  }
  m_cachedProgress = 0.0;
  m_cachedEasedProgress = 0.0;
}
void TransitionEngine::updateProgress() const {
  if (!m_active)
    return;
  auto now = std::chrono::steady_clock::now();
  long elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime)
          .count();
  m_cachedProgress =
      std::min(1.0, static_cast<double>(elapsed) / static_cast<double>(m_durationMs));
  m_cachedEasedProgress = m_easingFunc(m_cachedProgress);
  m_lastProgressUpdate = now;
}
double TransitionEngine::getProgress() const {
  updateProgress();
  return m_cachedProgress;
}
double TransitionEngine::getEasedProgress() const {
  updateProgress();
  return m_cachedEasedProgress;
}
bool TransitionEngine::render(cairo_t *cr, int width, int height) {
  return render(cr, width, height, TransitionParams{});
}
bool TransitionEngine::render(cairo_t *cr, int width, int height,
                              const TransitionParams &params) {
  if (!m_active)
    return false;
  updateProgress();
  const int w = (m_liveToWidth > 0) ? m_liveToWidth : width;
  const int h = (m_liveToHeight > 0) ? m_liveToHeight : height;
  if (m_liveToRender) {
    cairo_surface_t *toSurface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *toCr = cairo_create(toSurface);
    m_liveToRender(toCr, w, h);
    cairo_destroy(toCr);
    if (m_cachedProgress >= 1.0) {
      if (m_effect && m_from) {
        m_effect->render(cr, m_from, toSurface, 1.0, width, height, params);
      } else {
        cairo_set_source_surface(cr, toSurface, 0, 0);
        cairo_paint(cr);
      }
      cairo_surface_destroy(toSurface);
      FinishCallback callbackCopy = m_callback;
      stop();
      if (callbackCopy) {
        callbackCopy();
      }
      return false;
    }
    if (m_effect && m_from) {
      m_effect->render(cr, m_from, toSurface, m_cachedEasedProgress, width, height,
                      params);
    } else {
      cairo_set_source_surface(cr, toSurface, 0, 0);
      cairo_paint(cr);
    }
    cairo_surface_destroy(toSurface);
    return true;
  }
  if (m_cachedProgress >= 1.0) {
    if (m_effect && m_from && m_to) {
      m_effect->render(cr, m_from, m_to, 1.0, width, height, params);
    } else if (m_to) {
      cairo_set_source_surface(cr, m_to, 0, 0);
      cairo_paint(cr);
    }
    FinishCallback callbackCopy = m_callback;
    stop();
    if (callbackCopy) {
      callbackCopy();
    }
    return false;
  }
  if (m_effect && m_from && m_to) {
    m_effect->render(cr, m_from, m_to, m_cachedEasedProgress, width, height,
                     params);
  } else if (m_from && !m_to) {
    cairo_set_source_surface(cr, m_from, 0, 0);
    cairo_paint(cr);
  } else if (m_to && !m_from) {
    cairo_set_source_surface(cr, m_to, 0, 0);
    cairo_paint_with_alpha(cr, m_cachedEasedProgress);
  }
  return true;
}
}  
