#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include "WallpaperCard.hpp"
#include <functional>
#include <gtk/gtk.h>
#include <memory>
#include <vector>

namespace bwp::gui {

class WallpaperGrid {
public:
  WallpaperGrid();
  ~WallpaperGrid();

  GtkWidget *getWidget() const { return m_scrolledWindow; }

  void clear();
  void addWallpaper(const bwp::wallpaper::WallpaperInfo &info);

  // Filtering
  void filter(const std::string &query);
  void setSortOrder(int sortInfo); // Enum todo

  using SelectionCallback =
      std::function<void(const bwp::wallpaper::WallpaperInfo &)>;
  void setSelectionCallback(SelectionCallback callback);

private:
  GtkWidget *m_scrolledWindow;
  GtkWidget *m_flowBox;

  // Keep track of cards to manage memory or updates
  std::vector<std::unique_ptr<WallpaperCard>> m_cards;

  // Store data for filtering locally?
  struct Item {
    bwp::wallpaper::WallpaperInfo info;
    WallpaperCard *card;
  };
  std::vector<Item> m_items;
  SelectionCallback m_callback;
};

} // namespace bwp::gui
