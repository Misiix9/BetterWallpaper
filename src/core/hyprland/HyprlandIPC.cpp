#include "HyprlandIPC.hpp"
#include "../utils/Logger.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
namespace bwp::hyprland {
HyprlandIPC &HyprlandIPC::getInstance() {
  static HyprlandIPC instance;
  return instance;
}
HyprlandIPC::HyprlandIPC() {
  const char *sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (sig) {
    m_instanceSignature = sig;
  }
}
HyprlandIPC::~HyprlandIPC() { disconnect(); }
std::string HyprlandIPC::getSocketPath(const std::string &instanceSig,
                                       const char *socketName) {
  const char *runtimeDir = std::getenv("XDG_RUNTIME_DIR");
  if (!runtimeDir)
    return "";
  return std::string(runtimeDir) + "/hypr/" + instanceSig + "/" + socketName;
}
bool HyprlandIPC::isConnected() const { return !m_instanceSignature.empty(); }
bool HyprlandIPC::connect() {
  if (m_instanceSignature.empty()) {
    const char *sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!sig) {
      LOG_WARN("HYPRLAND_INSTANCE_SIGNATURE not found. Hyprland integration "
               "disabled.");
      return false;
    }
    m_instanceSignature = sig;
  }
  if (m_running)
    return true;
  m_running = true;
  m_eventThread = std::thread(&HyprlandIPC::eventLoop, this);
  LOG_INFO("Connected to Hyprland IPC.");
  return true;
}
void HyprlandIPC::disconnect() {
  m_running = false;
  if (m_eventThread.joinable()) {
    m_eventThread.join();
  }
}
std::string HyprlandIPC::dispatch(const std::string &command) {
  if (m_instanceSignature.empty())
    return "";
  std::string path = getSocketPath(m_instanceSignature, ".socket.sock");
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return "";
  struct sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
  if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sock);
    return "";
  }
  if (write(sock, command.c_str(), command.length()) < 0) {
    close(sock);
    return "";
  }
  std::string response;
  char buffer[4096];
  ssize_t n;
  while ((n = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[n] = '\0';
    response += buffer;
  }
  close(sock);
  return response;
}
void HyprlandIPC::setEventCallback(EventCallback callback) {
  std::lock_guard<std::mutex> lock(m_callbackMutex);
  m_callback = callback;
}
void HyprlandIPC::eventLoop() {
  std::string path = getSocketPath(m_instanceSignature, ".socket2.sock");
  while (m_running) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      close(sock);
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }
    std::string bufferStr;
    char buffer[1024];
    while (m_running) {
      struct pollfd pfd = {sock, POLLIN, 0};
      int ret = poll(&pfd, 1, 500); // 500ms timeout to check m_running
      if (ret == 0) continue; // timeout, re-check m_running
      if (ret < 0) break; // error
      ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
      if (n <= 0)
        break;  
      buffer[n] = '\0';
      bufferStr += buffer;
      size_t pos;
      while ((pos = bufferStr.find('\n')) != std::string::npos) {
        std::string line = bufferStr.substr(0, pos);
        bufferStr.erase(0, pos + 1);
        size_t sep = line.find(">>");
        if (sep != std::string::npos) {
          std::string event = line.substr(0, sep);
          std::string data = line.substr(sep + 2);
          std::lock_guard<std::mutex> lock(m_callbackMutex);
          if (m_callback) {
            m_callback(event, data);
          }
        }
      }
    }
    close(sock);
    std::this_thread::sleep_for(std::chrono::seconds(1));  
  }
}
}  
