#pragma once
#include "../../core/steam/SteamAPIClient.hpp"
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <functional>

namespace bwp::gui {

class WorkshopCard {
public:
  explicit WorkshopCard(const bwp::steam::WorkshopItem& item);
  ~WorkshopCard();

  GtkWidget* getWidget() const { return m_mainBox; }

  const bwp::steam::WorkshopItem& getItem() const { return m_item; }
  const std::string& getWorkshopId() const { return m_item.id; }

  /// Mark this card as already downloaded (swap download → checkmark)
  void setDownloaded(bool downloaded);
  bool isDownloaded() const { return m_isDownloaded; }

  /// Set the callback invoked when the download button is clicked
  using DownloadCallback = std::function<void(const std::string& id, const std::string& title)>;
  void setDownloadCallback(DownloadCallback cb) { m_downloadCb = std::move(cb); }

  /// Cancel in-flight thumbnail load
  void cancelThumbnailLoad();
  void releaseResources();

private:
  void loadThumbnailAsync(const std::string& url);

  GtkWidget* m_mainBox;
  GtkWidget* m_image;
  GtkWidget* m_overlay;
  GtkWidget* m_titleLabel;
  GtkWidget* m_actionBtn;       // Download or checkmark button
  GtkWidget* m_skeletonOverlay;

  bwp::steam::WorkshopItem m_item;
  bool m_isDownloaded = false;
  DownloadCallback m_downloadCb;
  std::shared_ptr<bool> m_aliveToken;
};

} // namespace bwp::gui
