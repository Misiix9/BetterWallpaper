#pragma once
#include "Easing.hpp"
#include "TransitionEffect.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
namespace bwp::transition {
class TransitionEngine {
public:
  using FinishCallback = std::function<void()>;
  /** Renders the current frame of the "to" wallpaper (called each frame; no capture). */
  using LiveToRenderFunc = std::function<void(cairo_t *cr, int width, int height)>;
  TransitionEngine();
  ~TransitionEngine();
  void start(cairo_surface_t *from, cairo_surface_t *to,
             std::shared_ptr<TransitionEffect> effect, long durationMs,
             const std::string &easingName = "easeInOut",
             FinishCallback callback = nullptr);
  void startWithEasing(cairo_surface_t *from, cairo_surface_t *to,
                       std::shared_ptr<TransitionEffect> effect,
                       long durationMs, Easing::EasingFunc easingFunc,
                       FinishCallback callback = nullptr);
  /** Start transition with live "to": only "from" is a captured frame; "to" is rendered each frame (wallpaper B plays during transition). */
  void startWithLiveTo(cairo_surface_t *from, LiveToRenderFunc liveToRender,
                       int width, int height,
                       std::shared_ptr<TransitionEffect> effect, long durationMs,
                       const std::string &easingName,
                       FinishCallback callback);
  void preload(cairo_surface_t *nextSurface);
  void clearPreload();
  void stop();
  bool render(cairo_t *cr, int width, int height);
  bool render(cairo_t *cr, int width, int height,
              const TransitionParams &params);
  bool isActive() const { return m_active; }
  double getProgress() const;
  double getEasedProgress() const;
  void setTargetFps(int fps) { m_targetFps = fps; }
  std::chrono::milliseconds getFrameInterval() const {
    return std::chrono::milliseconds(1000 / m_targetFps);
  }
private:
  bool m_active = false;
  cairo_surface_t *m_from = nullptr;
  cairo_surface_t *m_to = nullptr;
  LiveToRenderFunc m_liveToRender;
  int m_liveToWidth = 0;
  int m_liveToHeight = 0;
  cairo_surface_t *m_preloaded = nullptr;
  std::shared_ptr<TransitionEffect> m_effect;
  std::chrono::steady_clock::time_point m_startTime;
  long m_durationMs = 500;
  Easing::EasingFunc m_easingFunc = Easing::easeInOutQuad;
  int m_targetFps = 60;
  mutable double m_cachedProgress = 0.0;
  mutable double m_cachedEasedProgress = 0.0;
  mutable std::chrono::steady_clock::time_point m_lastProgressUpdate;
  FinishCallback m_callback;
  void updateProgress() const;
};
}  
