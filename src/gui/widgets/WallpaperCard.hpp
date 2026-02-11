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
  void updateThumbnail(const std::string &path);  
  void setFavorite(bool favorite);
  void setInfo(const bwp::wallpaper::WallpaperInfo &info);
  void updateMetadata(const bwp::wallpaper::WallpaperInfo &info);
  const bwp::wallpaper::WallpaperInfo &getInfo() const { return m_info; }
  void showSkeleton();
  void hideSkeleton();
  void cancelThumbnailLoad();
  void releaseResources();
  void setHighlight(const std::string &query);
private:
  GtkWidget *m_mainBox;
  GtkWidget *m_image;
  GtkWidget *m_titleLabel;
  GtkWidget *m_overlay;
  GtkWidget *m_favoriteBtn;
  GtkWidget *m_skeletonOverlay;  
  GtkWidget *m_scanOverlay;      
  GtkWidget *m_autoTagBadge;     
  GtkWidget *m_typeBadge = nullptr;  
  bwp::wallpaper::WallpaperInfo m_info;
  std::shared_ptr<bool> m_aliveToken;
  bool m_isLoading = true;
  guint m_thumbnailSourceId = 0;  
};
}  
