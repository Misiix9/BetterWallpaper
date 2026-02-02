#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include <functional>
#include <gtk/gtk.h>
#include <memory>
#include <string>

namespace bwp::gui {

class WallpaperCard {
public:
  WallpaperCard(const bwp::wallpaper::WallpaperInfo &info);
  ~WallpaperCard();

  GtkWidget *getWidget() const { return m_mainBox; }

  void updateThumbnail(const std::string &path); // Load from file/cache

  void setFavorite(bool favorite);
  void setInfo(const bwp::wallpaper::WallpaperInfo &info);
  const bwp::wallpaper::WallpaperInfo &getInfo() const { return m_info; }

  // Skeleton loading support
  void showSkeleton();
  void hideSkeleton();

  // Set wallpaper callback for context menu
  void setWallpaperCallback(std::function<void(const std::string &)> cb) {
    m_setWallpaperCallback = cb;
  }

private:
  void setupActions();
  void setupContextMenu();
  void showContextMenu(double x, double y);

  GtkWidget *m_mainBox;
  GtkWidget *m_image;
  GtkWidget *m_titleLabel;
  GtkWidget *m_overlay;
  GtkWidget *m_favoriteBtn;
  GtkWidget *m_skeletonOverlay; // Skeleton placeholder for loading state
  GtkWidget *m_scanOverlay; // Spinner container
  GtkWidget *m_autoTagBadge; // Icon
  GtkWidget *m_contextMenu = nullptr;

  bwp::wallpaper::WallpaperInfo m_info;
  std::shared_ptr<bool> m_aliveToken;
  std::function<void(const std::string &)> m_setWallpaperCallback;
  bool m_isLoading = true;
};

} // namespace bwp::gui
