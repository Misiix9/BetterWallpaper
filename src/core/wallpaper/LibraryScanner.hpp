#pragma once
#include <atomic>
#include <filesystem>
#include <functional>
#ifdef _WIN32
typedef void* gboolean;
typedef void* gpointer;
#else
#include <gtk/gtk.h>
#endif
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace bwp::wallpaper {

/**
 * @brief Progress information for library scanning
 */
struct ScanProgress {
  int filesScanned = 0;
  int filesFound = 0;
  int totalEstimate = 0; // Estimated total files (may be 0 if unknown)
  std::string currentPath;
  bool isComplete = false;

  [[nodiscard]] float getPercentage() const {
    if (totalEstimate <= 0)
      return -1.0f; // Indeterminate
    return static_cast<float>(filesScanned) /
           static_cast<float>(totalEstimate) * 100.0f;
  }
};

class LibraryScanner {
public:
  static LibraryScanner &getInstance();

  // Start scanning in background
  void scan(const std::vector<std::string> &paths);

  // Cancel an ongoing scan
  void cancelScan();

  // Wait for scan to complete
  void waitForCompletion();

  // Check specific file and add to library if new
  void scanFile(const std::filesystem::path &path);
  bool scanWorkshopItem(const std::filesystem::path &dir);

  // Status
  [[nodiscard]] bool isScanning() const { return m_scanning; }
  [[nodiscard]] ScanProgress getProgress() const;

  // Callbacks - called from main GTK thread via g_idle_add
  using ScanCallback = std::function<void(const std::string &path)>;
  using ProgressCallback = std::function<void(const ScanProgress &progress)>;
  using CompletionCallback = std::function<void(int totalFound)>;

  void setCallback(ScanCallback callback);
  void setProgressCallback(ProgressCallback callback);
  void setCompletionCallback(CompletionCallback callback);

private:
  LibraryScanner();
  ~LibraryScanner();

  void runScan(std::vector<std::string> paths);
  void reportProgress();
  void notifyCompletion();

  // Static callbacks for g_idle_add
  static gboolean idleProgress(gpointer data);
  static gboolean idleCompletion(gpointer data);
  static gboolean idleFileFound(gpointer data);

  std::atomic<bool> m_scanning{false};
  std::atomic<bool> m_cancelRequested{false};
  std::thread m_scanThread;

  // Callbacks
  ScanCallback m_callback;
  ProgressCallback m_progressCallback;
  CompletionCallback m_completionCallback;

  // Progress tracking
  mutable std::mutex m_mutex;
  ScanProgress m_progress;
};

} // namespace bwp::wallpaper
