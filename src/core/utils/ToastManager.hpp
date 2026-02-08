#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace bwp::core::utils {

enum class ToastType {
  Info,
  Success,
  Warning,
  Error
};

struct ToastAction {
  std::string label;
  std::function<void()> callback;
};

struct ToastRequest {
  std::string message;
  ToastType type = ToastType::Info;
  int durationMs = 5000;  // 0 = no auto-dismiss (e.g., save/discard)
  std::vector<ToastAction> actions;
};

class ToastManager {
public:
  static ToastManager &getInstance();

  // Simple message toast (backward compatible)
  using ToastCallback = std::function<void(const std::string &message)>;
  void showToast(const std::string &message);
  void setCallback(ToastCallback callback);

  // Extended toast with type and actions
  using ExtendedToastCallback = std::function<void(const ToastRequest &request)>;
  void showToast(const ToastRequest &request);
  void setExtendedCallback(ExtendedToastCallback callback);

  // Convenience methods
  void showSuccess(const std::string &message);
  void showError(const std::string &message);
  void showWarning(const std::string &message);
  void showInfo(const std::string &message);

private:
  ToastManager() = default;
  ~ToastManager() = default;

  ToastCallback m_callback;
  ExtendedToastCallback m_extendedCallback;
  std::mutex m_mutex;
};

} // namespace bwp::core::utils
