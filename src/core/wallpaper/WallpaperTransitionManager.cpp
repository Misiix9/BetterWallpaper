#include "WallpaperTransitionManager.hpp"
#include "../utils/Logger.hpp"
#include <chrono>
#include <csignal>
#include <ctime>
namespace bwp::wallpaper {
WallpaperTransitionManager &WallpaperTransitionManager::getInstance() {
  static WallpaperTransitionManager instance;
  return instance;
}
WallpaperTransitionManager::WallpaperTransitionManager() { loadSettings(); }
void WallpaperTransitionManager::loadSettings() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  std::lock_guard<std::mutex> lock(m_mutex);

  // Default to true
  m_settings.enabled = true;
  try {
    // Try to read the whole object to check for existence
    auto transObj = conf.get<nlohmann::json>("transitions");
    if (transObj.is_object() && transObj.contains("enabled")) {
      m_settings.enabled = transObj["enabled"].get<bool>();
    } else {
      // Key missing, force true
      m_settings.enabled = true;
      // Optional: write back to config?
      // conf.set("transitions.enabled", true);
    }
  } catch (...) {
    // If "transitions" object misses, default to true
    m_settings.enabled = true;
  }

  m_settings.durationMs = conf.get<int>("transitions.duration_ms", 500);
  m_settings.easingName =
      conf.get<std::string>("transitions.easing", "easeInOut");
  m_settings.effectName =
      conf.get<std::string>("transitions.default_effect", "Fade");

  LOG_INFO("Transition settings loaded: enabled=" +
           std::string(m_settings.enabled ? "true" : "false") +
           ", effect=" + m_settings.effectName +
           ", duration=" + std::to_string(m_settings.durationMs) + "ms");
}
WallpaperTransitionManager::TransitionSettings
WallpaperTransitionManager::getSettings() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_settings;
}
void WallpaperTransitionManager::startTransition(
    const std::string &monitorName, std::shared_ptr<WallpaperWindow> oldWindow,
    std::shared_ptr<WallpaperWindow> newWindow, CompletionCallback onComplete) {
  loadSettings();
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_transitions.count(monitorName) && m_transitions[monitorName].active) {
    if (m_transitions[monitorName].timerId > 0) {
      g_source_remove(m_transitions[monitorName].timerId);
    }
    LOG_WARN("Canceling existing transition for monitor: " + monitorName);
  }
  if (!m_settings.enabled) {
    LOG_INFO("Transitions disabled, completing immediately");
    if (oldWindow) {
      oldWindow->hide();
    }
    if (newWindow) {
      newWindow->setOpacity(1.0);
      newWindow->show();
    }
    if (onComplete) {
      onComplete(true);
    }
    return;
  }
  TransitionState state;
  state.active = true;
  state.progress = 0.0;
  state.startTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
  state.oldWindow = oldWindow;
  state.newWindow = newWindow;
  state.onComplete = onComplete;
  state.isWaitingForReady = true; // Start by waiting
  state.readyWaitStartTimeMs = state.startTimeMs;
  state.isOverlapDelaying = false;
  state.overlapStartTimeMs = 0;
  if (newWindow) {
    newWindow->setOpacity(0.0);
    newWindow->show();
  }
  if (oldWindow) {
    oldWindow->setOpacity(1.0);
  }
  m_transitions[monitorName] = state;
  LOG_INFO("Starting transition for monitor: " + monitorName +
           " (duration: " + std::to_string(m_settings.durationMs) + "ms)");
  struct CallbackData {
    WallpaperTransitionManager *manager;
    std::string monitorName;
  };
  CallbackData *data = new CallbackData{this, monitorName};
  constexpr int FRAME_INTERVAL_MS = 16;
  state.timerId = g_timeout_add(
      FRAME_INTERVAL_MS,
      [](gpointer userData) -> gboolean {
        CallbackData *cbData = static_cast<CallbackData *>(userData);
        bool shouldContinue = onAnimationTick(userData);
        if (!shouldContinue) {
          delete cbData;
        }
        return shouldContinue ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
      },
      data);
  m_transitions[monitorName].timerId = state.timerId;
}
void WallpaperTransitionManager::startExternalTransition(
    const std::string &monitorName, pid_t oldPid, pid_t newPid,
    CompletionCallback onComplete) {
  loadSettings();
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_settings.enabled) {
    if (oldPid > 0) {
      kill(oldPid, SIGTERM);
    }
    if (onComplete) {
      onComplete(true);
    }
    return;
  }
  TransitionState state;
  state.active = true;
  state.progress = 0.0;
  state.startTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
  state.oldPid = oldPid;
  state.newPid = newPid;
  state.onComplete = onComplete;
  m_transitions[monitorName] = state;
  LOG_INFO("Starting external transition for monitor: " + monitorName +
           " (old PID: " + std::to_string(oldPid) +
           ", new PID: " + std::to_string(newPid) + ")");
  struct CallbackData {
    WallpaperTransitionManager *manager;
    std::string monitorName;
  };
  CallbackData *data = new CallbackData{this, monitorName};
  guint timerId = g_timeout_add(
      m_settings.durationMs,
      [](gpointer userData) -> gboolean {
        CallbackData *cbData = static_cast<CallbackData *>(userData);
        cbData->manager->finishTransition(cbData->monitorName, true);
        delete cbData;
        return G_SOURCE_REMOVE;
      },
      data);
  m_transitions[monitorName].timerId = timerId;
}
gboolean WallpaperTransitionManager::onAnimationTick(gpointer data) {
  struct CallbackData {
    WallpaperTransitionManager *manager;
    std::string monitorName;
  };
  CallbackData *cbData = static_cast<CallbackData *>(data);
  WallpaperTransitionManager *manager = cbData->manager;
  const std::string &monitorName = cbData->monitorName;
  std::unique_lock<std::mutex> lock(manager->m_mutex);
  auto it = manager->m_transitions.find(monitorName);
  if (it == manager->m_transitions.end() || !it->second.active) {
    return G_SOURCE_REMOVE;
  }
  TransitionState &state = it->second;
  int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();

  // Phase 0: Wait for Ready
  if (state.isWaitingForReady) {
    bool isReady = false;
    if (state.newWindow) {
      if (auto renderer = state.newWindow->getRenderer()) {
        isReady = renderer->isReady();
      } else {
        isReady = true; // No renderer? Just proceed.
      }
    } else {
      isReady = true; // No new window? Just proceed.
    }

    // Timeout safety (5 seconds)
    if (nowMs - state.readyWaitStartTimeMs > 5000) {
      LOG_WARN("Timed out waiting for renderer ready, forcing transition");
      isReady = true;
    }

    if (!isReady) {
      return G_SOURCE_CONTINUE; // Keep waiting, don't advance fade
    }

    // Ready to start fade
    state.isWaitingForReady = false;
    state.startTimeMs = nowMs; // Reset start time for fade animation
    LOG_INFO("Renderer ready, starting fade transition");
  }

  // Phase 1: Fade Animation
  if (!state.isOverlapDelaying) {
    int64_t elapsedMs = nowMs - state.startTimeMs;
    double rawProgress =
        static_cast<double>(elapsedMs) / manager->m_settings.durationMs;
    rawProgress = std::min(1.0, std::max(0.0, rawProgress));
    auto easingFunc =
        bwp::transition::Easing::getByName(manager->m_settings.easingName);
    double easedProgress = easingFunc(rawProgress);
    state.progress = easedProgress;
    if (state.newWindow) {
      state.newWindow->setOpacity(easedProgress);
    }
    if (rawProgress >= 1.0) {
      // Fade complete, enter Overlap Delay
      state.isOverlapDelaying = true;
      state.overlapStartTimeMs = nowMs;
      if (state.newWindow)
        state.newWindow->setOpacity(1.0); // Ensure full opacity
      LOG_INFO("Fade complete, starting 0.8s overlap delay");
    }
    return G_SOURCE_CONTINUE;
  }

  // Phase 2: Overlap Delay (~0.8 seconds)
  if (state.isOverlapDelaying) {
    if (nowMs - state.overlapStartTimeMs >= 800) {
      // Delay complete
      std::string monName = monitorName;
      lock.unlock();
      manager->finishTransition(monName, true);
      return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
  }

  return G_SOURCE_CONTINUE;
}
void WallpaperTransitionManager::applyWindowOpacity(
    std::shared_ptr<WallpaperWindow> window, double opacity) {
  if (window) {
    window->setOpacity(opacity);
  }
}
void WallpaperTransitionManager::cancelTransition(
    const std::string &monitorName) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_transitions.find(monitorName);
  if (it != m_transitions.end() && it->second.active) {
    if (it->second.timerId > 0) {
      g_source_remove(it->second.timerId);
    }
    it->second.active = false;
    LOG_INFO("Transition canceled for monitor: " + monitorName);
  }
}
void WallpaperTransitionManager::finishTransition(
    const std::string &monitorName, bool success) {
  CompletionCallback callback;
  std::shared_ptr<WallpaperWindow> oldWindow;
  pid_t oldPid = 0;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_transitions.find(monitorName);
    if (it == m_transitions.end()) {
      return;
    }
    TransitionState &state = it->second;
    state.active = false;
    if (state.newWindow) {
      state.newWindow->setOpacity(1.0);
    }
    oldWindow = state.oldWindow;
    oldPid = state.oldPid;
    callback = state.onComplete;
    state.timerId = 0;
  }
  if (oldWindow) {
    oldWindow->hide();
  }
  if (oldPid > 0) {
    LOG_INFO("Killing old wallpaper process: " + std::to_string(oldPid));
    kill(oldPid, SIGTERM);
  }
  LOG_INFO("Transition completed for monitor: " + monitorName);
  if (callback) {
    callback(success);
  }
  // Clean up transition state to release shared_ptrs to old windows/renderers
  {
    std::lock_guard<std::mutex> lock2(m_mutex);
    m_transitions.erase(monitorName);
  }
}
bool WallpaperTransitionManager::isTransitioning(
    const std::string &monitorName) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_transitions.find(monitorName);
  return it != m_transitions.end() && it->second.active;
}
double
WallpaperTransitionManager::getProgress(const std::string &monitorName) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_transitions.find(monitorName);
  if (it != m_transitions.end()) {
    return it->second.progress;
  }
  return 0.0;
}
} // namespace bwp::wallpaper
