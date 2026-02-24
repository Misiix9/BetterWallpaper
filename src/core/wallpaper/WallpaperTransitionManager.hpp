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
class WallpaperTransitionManager {
public:
  static WallpaperTransitionManager &getInstance();
  struct TransitionSettings {
    bool enabled = true;
    std::string effectName = "Fade";
    int durationMs = 500;
    std::string easingName = "easeInOut";
  };
  using CompletionCallback = std::function<void(bool success)>;
  using ReadyCallback = std::function<void()>;
  void loadSettings();
  TransitionSettings getSettings() const;
  void startTransition(const std::string &monitorName,
                       std::shared_ptr<WallpaperWindow> oldWindow,
                       std::shared_ptr<WallpaperWindow> newWindow,
                       CompletionCallback onComplete = nullptr);
  void startExternalTransition(const std::string &monitorName, pid_t oldPid,
                               pid_t newPid,
                               CompletionCallback onComplete = nullptr);
  void cancelTransition(const std::string &monitorName);
  bool isTransitioning(const std::string &monitorName) const;
  double getProgress(const std::string &monitorName) const;

private:
  WallpaperTransitionManager();
  ~WallpaperTransitionManager() = default;
  WallpaperTransitionManager(const WallpaperTransitionManager &) = delete;
  WallpaperTransitionManager &
  operator=(const WallpaperTransitionManager &) = delete;
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
    // New fields for seamless transition logic
    bool isWaitingForReady = false;
    int64_t readyWaitStartTimeMs = 0;
    bool isOverlapDelaying = false;
    int64_t overlapStartTimeMs = 0;
  };
  static gboolean onAnimationTick(gpointer data);
  void applyWindowOpacity(std::shared_ptr<WallpaperWindow> window,
                          double opacity);
  void finishTransition(const std::string &monitorName, bool success);
  mutable std::mutex m_mutex;
  TransitionSettings m_settings;
  std::unordered_map<std::string, TransitionState> m_transitions;
};
} // namespace bwp::wallpaper
