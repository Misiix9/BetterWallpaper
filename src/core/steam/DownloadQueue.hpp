#pragma once
#include "SteamWorkshopClient.hpp"
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace bwp::steam {

// Download queue item
struct QueueItem {
  std::string workshopId;
  std::string title;
  DownloadProgress progress;

  enum class Status {
    Pending,
    Downloading,
    Paused,
    Completed,
    Failed
  } status = Status::Pending;

  std::string errorMessage;
};

/**
 * Manages a queue of workshop downloads.
 * Allows adding, removing, pausing, and cancelling downloads.
 */
class DownloadQueue {
public:
  using QueueChangeCallback =
      std::function<void(const std::vector<QueueItem> &queue)>;
  using ProgressCallback = std::function<void(
      const std::string &workshopId, const DownloadProgress &progress)>;
  using CompleteCallback = std::function<void(
      const std::string &workshopId, bool success, const std::string &path)>;

  static DownloadQueue &getInstance();

  // Queue management
  void addToQueue(const std::string &workshopId, const std::string &title = "");
  void removeFromQueue(const std::string &workshopId);
  void clearQueue();
  void moveToFront(const std::string &workshopId);

  // Control
  void startQueue();
  void pauseQueue();
  void resumeQueue();
  void cancelCurrent();

  // State
  std::vector<QueueItem> getQueue() const;
  bool isProcessing() const { return m_processing; }
  size_t queueSize() const;
  QueueItem *getCurrentItem();

  // Callbacks
  void setQueueChangeCallback(QueueChangeCallback callback) {
    m_queueChangeCallback = callback;
  }
  void setProgressCallback(ProgressCallback callback) {
    m_progressCallback = callback;
  }
  void setCompleteCallback(CompleteCallback callback) {
    m_completeCallback = callback;
  }

  // Persistence
  void loadFromConfig();
  void saveToConfig();

private:
  DownloadQueue() = default;
  ~DownloadQueue() = default;

  DownloadQueue(const DownloadQueue &) = delete;
  DownloadQueue &operator=(const DownloadQueue &) = delete;

  void processNext();
  void notifyQueueChange();

  std::deque<QueueItem> m_queue;
  mutable std::mutex m_mutex;

  bool m_processing = false;
  bool m_paused = false;

  QueueChangeCallback m_queueChangeCallback;
  ProgressCallback m_progressCallback;
  CompleteCallback m_completeCallback;
};

} // namespace bwp::steam
