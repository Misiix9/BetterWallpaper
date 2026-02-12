#pragma once
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace bwp::steam {

struct DownloadProgress {
  std::string workshopId;
  std::string title;
  float percent = 0.0f;
  std::string speed;
};

struct QueueItem {
  std::string workshopId;
  std::string title;
  std::string thumbnailUrl;
  std::string author;
  int votesUp = 0;
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
class DownloadQueue {
public:
  using QueueChangeCallback =
      std::function<void(const std::vector<QueueItem> &queue)>;
  using ProgressCallback = std::function<void(
      const std::string &workshopId, const DownloadProgress &progress)>;
  using CompleteCallback = std::function<void(
      const std::string &workshopId, bool success, const std::string &path)>;
  static DownloadQueue &getInstance();
  void addToQueue(const std::string &workshopId, const std::string &title = "",
                  const std::string &thumbnailUrl = "",
                  const std::string &author = "", int votesUp = 0);
  void removeFromQueue(const std::string &workshopId);
  void clearQueue();
  void moveToFront(const std::string &workshopId);
  void startQueue();
  void pauseQueue();
  void resumeQueue();
  void cancelCurrent();
  std::vector<QueueItem> getQueue() const;
  bool isProcessing() const { return m_processing; }
  size_t queueSize() const;
  QueueItem *getCurrentItem();
  void setQueueChangeCallback(QueueChangeCallback callback) {
    m_queueChangeCallback = callback;
  }
  void setProgressCallback(ProgressCallback callback) {
    m_progressCallback = callback;
  }
  void setCompleteCallback(CompleteCallback callback) {
    m_completeCallback = callback;
  }
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
