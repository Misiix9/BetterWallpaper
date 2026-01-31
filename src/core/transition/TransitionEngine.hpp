#pragma once
#include "Easing.hpp"
#include "TransitionEffect.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace bwp::transition {

/**
 * Enhanced TransitionEngine with easing support and optimizations.
 * Manages smooth transitions between wallpapers.
 */
class TransitionEngine {
public:
  using FinishCallback = std::function<void()>;

  TransitionEngine();
  ~TransitionEngine();

  /**
   * Start a transition with full configuration.
   * @param from Snapshot of the old wallpaper
   * @param to Snapshot of the new wallpaper
   * @param effect The effect to use
   * @param durationMs Duration in milliseconds
   * @param easingName Name of the easing function
   * @param callback Called when transition completes
   */
  void start(cairo_surface_t *from, cairo_surface_t *to,
             std::shared_ptr<TransitionEffect> effect, long durationMs,
             const std::string &easingName = "easeInOut",
             FinishCallback callback = nullptr);

  /**
   * Start a transition with custom easing function.
   */
  void startWithEasing(cairo_surface_t *from, cairo_surface_t *to,
                       std::shared_ptr<TransitionEffect> effect,
                       long durationMs, Easing::EasingFunc easingFunc,
                       FinishCallback callback = nullptr);

  /**
   * Preload the next wallpaper surface for faster transitions.
   * The surface is kept until needed or cleared.
   */
  void preload(cairo_surface_t *nextSurface);

  /**
   * Clear any preloaded surface.
   */
  void clearPreload();

  /**
   * Stop the current transition immediately.
   */
  void stop();

  /**
   * Update and render the current frame.
   * @param cr Cairo context
   * @param width Output width
   * @param height Output height
   * @return true if transition is still active, false if finished
   */
  bool render(cairo_t *cr, int width, int height);

  /**
   * Render with custom parameters.
   */
  bool render(cairo_t *cr, int width, int height,
              const TransitionParams &params);

  bool isActive() const { return m_active; }

  /**
   * Get current progress (0.0 to 1.0).
   */
  double getProgress() const;

  /**
   * Get current eased progress.
   */
  double getEasedProgress() const;

  /**
   * Set target FPS for frame timing calculations.
   */
  void setTargetFps(int fps) { m_targetFps = fps; }

  /**
   * Get recommended frame interval based on target FPS.
   */
  std::chrono::milliseconds getFrameInterval() const {
    return std::chrono::milliseconds(1000 / m_targetFps);
  }

private:
  bool m_active = false;
  cairo_surface_t *m_from = nullptr;
  cairo_surface_t *m_to = nullptr;
  cairo_surface_t *m_preloaded = nullptr;

  std::shared_ptr<TransitionEffect> m_effect;
  std::chrono::steady_clock::time_point m_startTime;
  long m_durationMs = 500;

  Easing::EasingFunc m_easingFunc = Easing::easeInOutQuad;
  int m_targetFps = 60;

  // Cached values for current frame
  mutable double m_cachedProgress = 0.0;
  mutable double m_cachedEasedProgress = 0.0;
  mutable std::chrono::steady_clock::time_point m_lastProgressUpdate;

  FinishCallback m_callback;

  void updateProgress() const;
};

} // namespace bwp::transition
