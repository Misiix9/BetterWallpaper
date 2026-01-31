#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
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

  bwp::wallpaper::WallpaperInfo m_info;
  std::shared_ptr<bool> m_aliveToken;
  bool m_isLoading = true;
};

} // namespace bwp::gui
