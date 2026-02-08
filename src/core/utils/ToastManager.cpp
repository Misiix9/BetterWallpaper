#include "ToastManager.hpp"
#include "Logger.hpp"

namespace bwp::core::utils {

ToastManager &ToastManager::getInstance() {
  static ToastManager instance;
  return instance;
}

void ToastManager::showToast(const std::string &message) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_extendedCallback) {
    ToastRequest req;
    req.message = message;
    req.type = ToastType::Info;
    req.durationMs = 5000;
    m_extendedCallback(req);
  } else if (m_callback) {
    m_callback(message);
  } else {
    LOG_WARN("ToastManager: No callback registered, ignoring toast: " +
             message);
  }
}

void ToastManager::showToast(const ToastRequest &request) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_extendedCallback) {
    m_extendedCallback(request);
  } else if (m_callback) {
    m_callback(request.message);
  } else {
    LOG_WARN("ToastManager: No callback registered, ignoring toast: " +
             request.message);
  }
}

void ToastManager::setCallback(ToastCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_callback = callback;
}

void ToastManager::setExtendedCallback(ExtendedToastCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_extendedCallback = callback;
}

void ToastManager::showSuccess(const std::string &message) {
  ToastRequest req;
  req.message = message;
  req.type = ToastType::Success;
  req.durationMs = 4000;
  showToast(req);
}

void ToastManager::showError(const std::string &message) {
  ToastRequest req;
  req.message = message;
  req.type = ToastType::Error;
  req.durationMs = 8000;
  showToast(req);
}

void ToastManager::showWarning(const std::string &message) {
  ToastRequest req;
  req.message = message;
  req.type = ToastType::Warning;
  req.durationMs = 6000;
  showToast(req);
}

void ToastManager::showInfo(const std::string &message) {
  ToastRequest req;
  req.message = message;
  req.type = ToastType::Info;
  req.durationMs = 5000;
  showToast(req);
}

} // namespace bwp::core::utils
