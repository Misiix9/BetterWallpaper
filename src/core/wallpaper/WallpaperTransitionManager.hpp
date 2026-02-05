#pragma once

#include "../config/ConfigManager.hpp"
#include "../transition/Easing.hpp"
#include "../transition/TransitionEffect.hpp"
#include "WallpaperRenderer.hpp"
#include "WallpaperWindow.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace bwp::wallpaper {

/**
 * WallpaperTransitionManager coordinates the transition from one wallpaper to
 * another. It ensures:
 * - Old wallpaper stays visible until new one is fully loaded
 * - New wallpaper appears IN FRONT of the old one (higher z-index)
 * - New wallpaper fades in (opacity 0 → 1) over the old one
 * - Old wallpaper is destroyed only after transition completes
 *
 * Works with both internal renderers (Static/Video) and external processes
 * (linux-wallpaperengine).
 */
class WallpaperTransitionManager {
public:
  static WallpaperTransitionManager &getInstance();

  // Settings from config
  struct TransitionSettings {
    bool enabled = true;
    std::string effectName = "Fade";
    int durationMs = 500;
    std::string easingName = "easeInOut";
  };

  // Callback types
  using CompletionCallback = std::function<void(bool success)>;
  using ReadyCallback = std::function<void()>;

  /**
   * Load transition settings from ConfigManager
   */
  void loadSettings();

  /**
   * Get current transition settings
   */
  TransitionSettings getSettings() const;

  /**
   * Start a transition from old wallpaper to new wallpaper.
   *
   * @param monitorName Target monitor
   * @param oldWindow The current wallpaper window (will be destroyed after
   * transition)
   * @param newWindow The new wallpaper window (starts invisible, fades in)
   * @param onComplete Callback when transition finishes
   */
  void startTransition(const std::string &monitorName,
                       std::shared_ptr<WallpaperWindow> oldWindow,
                       std::shared_ptr<WallpaperWindow> newWindow,
                       CompletionCallback onComplete = nullptr);

  /**
   * Start a transition for linux-wallpaperengine wallpapers.
   * Uses an overlay window to handle the fade since we can't control
   * the external process's opacity directly.
   *
   * @param monitorName Target monitor
   * @param oldPid PID of the old linux-wallpaperengine process
   * @param newPid PID of the new linux-wallpaperengine process
   * @param onComplete Callback when transition finishes
   */
  void startExternalTransition(const std::string &monitorName, pid_t oldPid,
                               pid_t newPid,
                               CompletionCallback onComplete = nullptr);

  /**
   * Cancel any ongoing transition for a monitor
   */
  void cancelTransition(const std::string &monitorName);

  /**
   * Check if a transition is in progress for a monitor
   */
  bool isTransitioning(const std::string &monitorName) const;

  /**
   * Get current transition progress (0.0 to 1.0) for a monitor
   */
  double getProgress(const std::string &monitorName) const;

private:
  WallpaperTransitionManager();
  ~WallpaperTransitionManager() = default;

  WallpaperTransitionManager(const WallpaperTransitionManager &) = delete;
  WallpaperTransitionManager &
  operator=(const WallpaperTransitionManager &) = delete;

  // Internal transition state for each monitor
  struct TransitionState {
    bool active = false;
    double progress = 0.0;
    int64_t startTimeMs = 0;
    std::shared_ptr<WallpaperWindow> oldWindow;
    std::shared_ptr<WallpaperWindow> newWindow;
    pid_t oldPid = 0;
    pid_t newPid = 0;
    CompletionCallback onComplete;
    guint timerId = 0;
  };

  // Animation tick handler
  static gboolean onAnimationTick(gpointer data);

  // Apply eased opacity to window (for internal renderers)
  void applyWindowOpacity(std::shared_ptr<WallpaperWindow> window,
                          double opacity);

  // Cleanup after transition
  void finishTransition(const std::string &monitorName, bool success);

  mutable std::mutex m_mutex;
  TransitionSettings m_settings;
  std::unordered_map<std::string, TransitionState> m_transitions;
};

} // namespace bwp::wallpaper
