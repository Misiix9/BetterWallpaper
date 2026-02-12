#pragma once
#include "../../core/steam/DownloadQueue.hpp"
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <memory>
#include <string>

namespace bwp::gui {

/**
 * DownloadIndicator — Floating download icon that appears
 * bottom-right during workshop downloads.
 * On hover, shows a popover with wallpaper thumbnail, title,
 * author, and progress.
 *
 * This is a pure display widget. Call updateProgress() and
 * downloadComplete() from external callbacks.
 */
class DownloadIndicator {
public:
  DownloadIndicator();
  ~DownloadIndicator();

  GtkWidget *getWidget() const { return m_overlay; }

  /// Update with current download state (call from main thread)
  void updateProgress(const std::string &workshopId, const std::string &title,
                      float percent);
  void downloadComplete(const std::string &workshopId, bool success);

  // Sync UI with queue state (called from main thread)
  void updateFromQueue(const std::vector<bwp::steam::QueueItem> &queue);

  using RetryCallback = std::function<void(const std::string &)>;
  void onRetry(RetryCallback cb);
  void hide();

  struct DownloadItem {
    std::string id;
    std::string title;
    std::string thumbnailUrl;
    std::string author;
    int votesUp = 0;
    float progress = 0.0f;
    bool finished = false;
    bool failed = false;

    // UI Cache
    std::shared_ptr<GdkTexture> texture;
    bool isLoading = false;
  };

  void clearHistory();

private:
  void setupUi();
  void refreshPopover();
  void addHistoryItem(const DownloadItem &item);
  void loadThumbnail(std::string id, std::string url);

  GtkWidget *m_overlay = nullptr;
  GtkWidget *m_button = nullptr; // Always visible button
  GtkWidget *m_popover = nullptr;

  // UI Containers within popover
  GtkWidget *m_activeBox = nullptr;
  GtkWidget *m_historyBox = nullptr;
  GtkWidget *m_emptyLabel = nullptr;

  // Data
  std::vector<DownloadItem> m_activeDownloads;
  std::vector<DownloadItem> m_history;
  RetryCallback m_retryCallback;
};

} // namespace bwp::gui
