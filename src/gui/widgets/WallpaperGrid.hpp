#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include <functional>
#include <gtk/gtk.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
namespace bwp::gui {
class WallpaperCard;
class WallpaperGrid {
public:
  WallpaperGrid();
  ~WallpaperGrid();
  enum class SortOrder {
    NameAsc,
    NameDesc,
    DateNewest,
    DateOldest,
    RatingDesc
  };
  GtkWidget *getWidget() const { return m_scrolledWindow; }
  void clear();
  void addWallpaper(const bwp::wallpaper::WallpaperInfo &info);
  void removeWallpaperById(const std::string &id);
  void updateWallpaperInStore(const bwp::wallpaper::WallpaperInfo &info);
  void notifyDataChanged();
  void filter(const std::string &query);
  void setFilterFavoritesOnly(bool onlyFavorites);
  void setFilterTag(const std::string &tag);
  void setFilterSource(const std::string &source);
  void setSortOrder(SortOrder sortInfo);
  using SelectionCallback =
      std::function<void(const bwp::wallpaper::WallpaperInfo &)>;
  void setSelectionCallback(SelectionCallback callback);
  using ReorderCallback = std::function<void(
      const std::string &sourceId, const std::string &targetId, bool after)>;
  void setReorderCallback(ReorderCallback callback);
  double getVScroll() const;
  void setVScroll(double value);
  void clearSelection();
private:
  GtkWidget *m_scrolledWindow;
  GtkWidget *m_gridView;
  GListStore *m_store;
  GtkFilterListModel *m_filterModel;
  GtkSortListModel *m_sortModel;
  GtkSingleSelection *m_selectionModel;
  GtkCustomFilter *m_filter;
  SelectionCallback m_callback;
  ReorderCallback m_reorderCallback;
  std::string m_filterQuery;
  bool m_filterFavorites = false;
  std::string m_filterTag;
  std::string m_filterSource = "all";
  SortOrder m_currentSort = SortOrder::NameAsc;
  std::unordered_set<std::string> m_existingPaths;
  std::unordered_map<std::string, WallpaperCard*> m_boundCards;
  void updateFilter();
  static void onSetup(GtkSignalListItemFactory *factory, GtkListItem *item,
                      gpointer user_data);
  static void onBind(GtkSignalListItemFactory *factory, GtkListItem *item,
                     gpointer user_data);
  static void onUnbind(GtkSignalListItemFactory *factory, GtkListItem *item,
                       gpointer user_data);
  static void onTeardown(GtkSignalListItemFactory *factory, GtkListItem *item,
                         gpointer user_data);
  static void onSelectionChanged(GtkSelectionModel *model, guint position,
                                 guint n_items, gpointer user_data);
};
}  
