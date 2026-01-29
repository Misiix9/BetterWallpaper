#pragma once
#include "../models/WallpaperObject.hpp"
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
  GtkWidget *m_gridView;

  // Data models
  GListStore *m_store;
  GtkFilterListModel *m_filterModel;
  GtkSortListModel *m_sortModel;
  GtkSingleSelection *m_selectionModel;
  GtkCustomFilter *m_filter;

  SelectionCallback m_callback;

  // Signal handlers for factory
  static void onSetup(GtkSignalListItemFactory *factory, GtkListItem *item,
                      gpointer user_data);
  static void onBind(GtkSignalListItemFactory *factory, GtkListItem *item,
                     gpointer user_data);
  static void onUnbind(GtkSignalListItemFactory *factory, GtkListItem *item,
                       gpointer user_data);
  static void onTeardown(GtkSignalListItemFactory *factory, GtkListItem *item,
                         gpointer user_data);

  // Selection change handler
  static void onSelectionChanged(GtkSelectionModel *model, guint position,
                                 guint n_items, gpointer user_data);
};

} // namespace bwp::gui
