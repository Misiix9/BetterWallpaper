#pragma once
#include "../models/WallpaperObject.hpp"
#include "TagEditorDialog.hpp"
#include <functional>
#include <gtk/gtk.h>
#include <memory>
#include <unordered_set>
#include <vector>

namespace bwp::gui {

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

  // Filtering
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
  ReorderCallback m_reorderCallback;
  std::unique_ptr<TagEditorDialog> m_tagEditor;

  // Filter state
  std::string m_filterQuery;
  bool m_filterFavorites = false;
  std::string m_filterTag;
  std::string m_filterSource = "all";
  SortOrder m_currentSort = SortOrder::NameAsc;

  // Duplicate tracking (faster than iterating store)
  std::unordered_set<std::string> m_existingPaths;

  // Signal handlers for factory
  void updateFilter();
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
