#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include <gtk/gtk.h>
#include <string>

namespace bwp::gui {

class WallpaperCard {
public:
  WallpaperCard(const bwp::wallpaper::WallpaperInfo &info);
  ~WallpaperCard();

  GtkWidget *getWidget() const { return m_mainBox; }

  void updateThumbnail(const std::string &path); // Load from file/cache

  void setFavorite(bool favorite);

private:
  void setupActions();
  void setupContextMenu();
  void showContextMenu(double x, double y);

  GtkWidget *m_mainBox;
  GtkWidget *m_image;
  GtkWidget *m_titleLabel;
  GtkWidget *m_overlay;
  GtkWidget *m_favoriteBtn;

  bwp::wallpaper::WallpaperInfo m_info;
};

} // namespace bwp::gui
