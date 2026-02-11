#include "DownloadQueue.hpp"
#include "SteamService.hpp"
#include "../config/ConfigManager.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
namespace bwp::steam {
DownloadQueue &DownloadQueue::getInstance() {
  static DownloadQueue instance;
  return instance;
}
void DownloadQueue::addToQueue(const std::string &workshopId,
                               const std::string &title) {
  bool shouldProcess = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &item : m_queue) {
      if (item.workshopId == workshopId) {
        LOG_WARN("Item already in queue: " + workshopId);
        return;
      }
    }
    QueueItem item;
    item.workshopId = workshopId;
    item.title = title.empty() ? ("Workshop Item " + workshopId) : title;
    item.status = QueueItem::Status::Pending;
    item.progress.workshopId = workshopId;
    item.progress.title = item.title;
    m_queue.push_back(item);
    LOG_INFO("Added to download queue: " + item.title);
    notifyQueueChange();
    saveToConfig();
    shouldProcess = !m_processing && !m_paused;
  }
  if (shouldProcess) {
    processNext();
  }
}
void DownloadQueue::removeFromQueue(const std::string &workshopId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_queue.empty() && m_queue.front().workshopId == workshopId &&
      m_queue.front().status == QueueItem::Status::Downloading) {
    LOG_WARN("Cannot remove item currently downloading. Cancel it first.");
    return;
  }
  auto it = std::remove_if(
      m_queue.begin(), m_queue.end(),
      [&](const QueueItem &item) { return item.workshopId == workshopId; });
  if (it != m_queue.end()) {
    m_queue.erase(it, m_queue.end());
    LOG_INFO("Removed from queue: " + workshopId);
    notifyQueueChange();
    saveToConfig();
  }
}
void DownloadQueue::clearQueue() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_processing) {
    SteamService::getInstance().cancelDownload();
    m_processing = false;
  }
  m_queue.clear();
  notifyQueueChange();
  saveToConfig();
}
void DownloadQueue::moveToFront(const std::string &workshopId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it =
      std::find_if(m_queue.begin(), m_queue.end(), [&](const QueueItem &item) {
        return item.workshopId == workshopId;
      });
  if (it != m_queue.end() && it != m_queue.begin()) {
    if (m_queue.front().status == QueueItem::Status::Downloading) {
      QueueItem item = *it;
      m_queue.erase(it);
      auto insertPos = m_queue.begin();
      ++insertPos;  
      m_queue.insert(insertPos, item);
    } else {
      QueueItem item = *it;
      m_queue.erase(it);
      m_queue.push_front(item);
    }
    notifyQueueChange();
    saveToConfig();
  }
}
void DownloadQueue::startQueue() {
  bool shouldProcess = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_paused = false;
    shouldProcess = !m_processing && !m_queue.empty();
  }
  if (shouldProcess) {
    processNext();
  }
}
void DownloadQueue::pauseQueue() {
  m_paused = true;
  LOG_INFO("Download queue paused");
}
void DownloadQueue::resumeQueue() {
  m_paused = false;
  if (!m_processing) {
    processNext();
  }
  LOG_INFO("Download queue resumed");
}
void DownloadQueue::cancelCurrent() {
  SteamService::getInstance().cancelDownload();
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_queue.empty() &&
      m_queue.front().status == QueueItem::Status::Downloading) {
    m_queue.front().status = QueueItem::Status::Failed;
    m_queue.front().errorMessage = "Cancelled by user";
  }
  m_processing = false;
  notifyQueueChange();
}
std::vector<QueueItem> DownloadQueue::getQueue() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return std::vector<QueueItem>(m_queue.begin(), m_queue.end());
}
size_t DownloadQueue::queueSize() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_queue.size();
}
QueueItem *DownloadQueue::getCurrentItem() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_queue.empty() &&
      m_queue.front().status == QueueItem::Status::Downloading) {
    return &m_queue.front();
  }
  return nullptr;
}
void DownloadQueue::processNext() {
  if (m_paused)
    return;
  
  std::string workshopId;
  std::string cachedTitle;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &item : m_queue) {
      if (item.status == QueueItem::Status::Pending) {
        item.status = QueueItem::Status::Downloading;
        m_processing = true;
        workshopId = item.workshopId;
        cachedTitle = item.title;
        notifyQueueChange();
        break;
      }
    }
    if (workshopId.empty()) {
      m_processing = false;
      return;
    }
  }
  // Mutex released — safe to call blocking download
  SteamService::getInstance().downloadWallpaper(
      workshopId,
      [this, workshopId, cachedTitle](float percent) {
        std::lock_guard<std::mutex> lock(m_mutex);
        DownloadProgress prog;
        prog.workshopId = workshopId;
        prog.title = cachedTitle;
        prog.percent = percent * 100.0f;
        
        for (auto &item : m_queue) {
          if (item.workshopId == workshopId) {
            item.progress = prog;
            break;
          }
        }
        if (m_progressCallback) {
          m_progressCallback(workshopId, prog);
        }
      },
      [this, workshopId](bool success) {
        {
          std::lock_guard<std::mutex> lock(m_mutex);
          for (auto &item : m_queue) {
            if (item.workshopId == workshopId) {
              item.status = success ? QueueItem::Status::Completed
                                    : QueueItem::Status::Failed;
              if (!success) {
                item.errorMessage = "Failed";
              }
              break;
            }
          }
          m_processing = false;
        }
        if (m_completeCallback) {
          m_completeCallback(workshopId, success, "");
        }
        notifyQueueChange();
        saveToConfig();
        processNext();
      });
}
void DownloadQueue::notifyQueueChange() {
  if (m_queueChangeCallback) {
    std::vector<QueueItem> copy(m_queue.begin(), m_queue.end());
    m_queueChangeCallback(copy);
  }
}
void DownloadQueue::loadFromConfig() {
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto queueJson = conf.get<std::vector<nlohmann::json>>("download_queue");
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queue.clear();
  for (const auto &j : queueJson) {
    QueueItem item;
    item.workshopId = j.value("workshopId", "");
    item.title = j.value("title", "");
    int statusInt = j.value("status", 0);
    item.status = static_cast<QueueItem::Status>(statusInt);
    if (item.status == QueueItem::Status::Downloading) {
      item.status = QueueItem::Status::Pending;
    }
    if (!item.workshopId.empty()) {
      m_queue.push_back(item);
    }
  }
}
void DownloadQueue::saveToConfig() {
  std::vector<nlohmann::json> queueJson;
  for (const auto &item : m_queue) {
    nlohmann::json j;
    j["workshopId"] = item.workshopId;
    j["title"] = item.title;
    j["status"] = static_cast<int>(item.status);
    queueJson.push_back(j);
  }
  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.set("download_queue", queueJson);
}
}  
