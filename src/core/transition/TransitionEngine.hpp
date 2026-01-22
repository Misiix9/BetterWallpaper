#pragma once
#include "TransitionEffect.hpp"
#include <chrono>
#include <functional>
#include <memory>

namespace bwp::transition {

class TransitionEngine {
public:
  using FinishCallback = std::function<void()>;

  TransitionEngine();
  ~TransitionEngine();

  /**
   * Start a transition.
   * @param from Snapshot of the old wallpaper
   * @param to Snapshot of the new wallpaper
   * @param effect The effect to use
   * @param durationMs Duration in milliseconds
   * @param callback Called when transition completes
   */
  void start(cairo_surface_t *from, cairo_surface_t *to,
             std::shared_ptr<TransitionEffect> effect, long durationMs,
             FinishCallback callback = nullptr);

  void stop();

  /**
   * Update and render the current frame.
   * @param cr Cairo context
   * @param width Output width
   * @param height Output height
   * @return true if transition is still active, false if finished
   */
  bool render(cairo_t *cr, int width, int height);

  bool isActive() const { return m_active; }

private:
  bool m_active = false;
  cairo_surface_t *m_from = nullptr;
  cairo_surface_t *m_to = nullptr;

  std::shared_ptr<TransitionEffect> m_effect;
  std::chrono::steady_clock::time_point m_startTime;
  long m_durationMs = 500;

  FinishCallback m_callback;
};

} // namespace bwp::transition
