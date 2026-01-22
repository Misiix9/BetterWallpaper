#pragma once
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace bwp::wallpaper {

class LibraryScanner {
public:
  static LibraryScanner &getInstance();

  // Start scanning in background
  void scan(const std::vector<std::string> &paths);

  // Check specific file and add to library if new
  void scanFile(const std::filesystem::path &path);

  // Status
  bool isScanning() const { return m_scanning; }

  using ScanCallback = std::function<void(const std::string &path)>;
  void setCallback(ScanCallback callback);

private:
  LibraryScanner();
  ~LibraryScanner();

  void runScan(std::vector<std::string> paths);

  std::atomic<bool> m_scanning = false;
  std::thread m_scanThread;
  ScanCallback m_callback;
  std::mutex m_mutex;
};

} // namespace bwp::wallpaper
