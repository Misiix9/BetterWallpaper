#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace bwp::hyprland {
class HyprlandIPC {
public:
  using EventCallback =
      std::function<void(const std::string &event, const std::string &data)>;
  static HyprlandIPC &getInstance();
  bool connect();
  void disconnect();
  bool isConnected() const;
  std::string dispatch(const std::string &command);
  void setEventCallback(EventCallback callback);
private:
  HyprlandIPC();
  ~HyprlandIPC();
  void eventLoop();
  std::string getSocketPath(const std::string &instanceSig,
                            const char *socketName);
  std::atomic<bool> m_running{false};
  std::thread m_eventThread;
  EventCallback m_callback;
  std::mutex m_callbackMutex;
  std::string m_instanceSignature;
};
}  
