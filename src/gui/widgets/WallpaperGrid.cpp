#include "WallpaperGrid.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/utils/StringUtils.hpp"
#include "WallpaperCard.hpp"
#include <filesystem>

namespace bwp::gui {

WallpaperGrid::WallpaperGrid() {
  // Scrolled Window
  m_scrolledWindow = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(m_scrolledWindow, TRUE);
  gtk_widget_set_hexpand(m_scrolledWindow, TRUE);

  // Data Model Pipeline
  // 1. Base Store
  m_store = g_list_store_new(BWP_TYPE_WALLPAPER_OBJECT);

  // 2. Filter Model
  m_filter =
      gtk_custom_filter_new(bwp_wallpaper_object_match, nullptr, nullptr);
  m_filterModel =
      gtk_filter_list_model_new(G_LIST_MODEL(m_store), GTK_FILTER(m_filter));

  // 3. Sort Model (Default sort)
  m_sortModel = gtk_sort_list_model_new(G_LIST_MODEL(m_filterModel), nullptr);

  // 4. Selection Model
  m_selectionModel = gtk_single_selection_new(G_LIST_MODEL(m_sortModel));

  // Note: autoselect is true by default, which selects the first item.
  // We might want to disable that if we want "no selection" initially,
  // but GtkSingleSelection implies one item is selected if not empty.
  // For no selection, GtkNoSelection usually used, but we want selection
  // capability. We can stick with SingleSelection for now.

  g_signal_connect(m_selectionModel, "selection-changed",
                   G_CALLBACK(onSelectionChanged), this);

  // Item Factory
  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(onSetup), this);
  g_signal_connect(factory, "bind", G_CALLBACK(onBind), this);
  g_signal_connect(factory, "unbind", G_CALLBACK(onUnbind), this);
  g_signal_connect(factory, "teardown", G_CALLBACK(onTeardown), this);

  // Grid View
  m_gridView =
      gtk_grid_view_new(GTK_SELECTION_MODEL(m_selectionModel), factory);

  // Card size about 160 + margins
  gtk_grid_view_set_max_columns(GTK_GRID_VIEW(m_gridView), 20); // Dynamic
  gtk_grid_view_set_min_columns(GTK_GRID_VIEW(m_gridView), 2);

  // Add CSS class for styling if needed
  gtk_widget_add_css_class(m_gridView, "wallpaper-grid");

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                m_gridView);
}

WallpaperGrid::~WallpaperGrid() {
  // GObjects are ref-counted, but we might need to unref models if we own the
  // initial ref GtkGridView takes ownership of the model passed to it? Usually
  // widgets just take a reference.

  // We created them, so we own a reference.
  g_object_unref(m_selectionModel);
  g_object_unref(m_sortModel);
  g_object_unref(m_filterModel);
  g_object_unref(m_filter);
  g_object_unref(m_store);
}

void WallpaperGrid::clear() { g_list_store_remove_all(m_store); }

void WallpaperGrid::addWallpaper(const bwp::wallpaper::WallpaperInfo &info) {
  BwpWallpaperObject *obj = bwp_wallpaper_object_new(info);
  g_list_store_append(m_store, obj);
  g_object_unref(obj);
}

void WallpaperGrid::filter(const std::string &query) {
  // We need to pass the query to the match function.
  // Since GtkCustomFilter callback assumes user_data is static or managed,
  // we need a way to pass the string.

  // A simple way is to recreate the filter, or set user_data.
  // But user_data in gtk_custom_filter_new is fixed.

  // Alternative: subclass GtkCustomFilter or use a persistent context.
  // For now, let's just create a new filter. It's cheap.

  // Note: Using a heap-allocated string for user_data
  char *q = query.empty() ? nullptr : g_strdup(query.c_str());

  auto matchFunc = [](gpointer item, gpointer user_data) -> gboolean {
    gboolean result = bwp_wallpaper_object_match(item, user_data);
    return result;
  };

  m_filter = gtk_custom_filter_new(matchFunc, q, g_free);
  gtk_filter_list_model_set_filter(m_filterModel, GTK_FILTER(m_filter));
}

void WallpaperGrid::setSortOrder(int /*sortInfo*/) {
  // Todo: Implement sort sorter
}

void WallpaperGrid::setSelectionCallback(SelectionCallback callback) {
  m_callback = callback;
}

// Static Callbacks

void WallpaperGrid::onSetup(GtkSignalListItemFactory * /*factory*/,
                            GtkListItem *item, gpointer /*user_data*/) {
  LOG_DEBUG("onSetup");
  // Create a WallpaperCard. Since we don't have info yet, create with dummy
  // info. But WallpaperCard expects valid info. We can construct with empty
  // info.
  bwp::wallpaper::WallpaperInfo dummy;
  WallpaperCard *card = new WallpaperCard(dummy);

  // Center alignment for the grid cell
  gtk_widget_set_halign(card->getWidget(), GTK_ALIGN_CENTER);
  gtk_widget_set_valign(card->getWidget(), GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(card->getWidget(), 8);
  gtk_widget_set_margin_end(card->getWidget(), 8);
  gtk_widget_set_margin_top(card->getWidget(), 8);
  gtk_widget_set_margin_bottom(card->getWidget(), 8);

  // Attach card pointer to widget so we can retrieve it
  g_object_set_data(G_OBJECT(card->getWidget()), "card-ptr", card);

  gtk_list_item_set_child(item, card->getWidget());
}

void WallpaperGrid::onBind(GtkSignalListItemFactory * /*factory*/,
                           GtkListItem *item, gpointer /*user_data*/) {
  GtkWidget *widget = gtk_list_item_get_child(item);
  WallpaperCard *card = static_cast<WallpaperCard *>(
      g_object_get_data(G_OBJECT(widget), "card-ptr"));

  BwpWallpaperObject *obj = BWP_WALLPAPER_OBJECT(gtk_list_item_get_item(item));
  const bwp::wallpaper::WallpaperInfo *info =
      bwp_wallpaper_object_get_info(obj);

  if (card && info) {
    LOG_DEBUG("onBind: " + info->path);
    card->setInfo(*info);
  } else {
    LOG_WARN("onBind: card or info is null");
  }
}

void WallpaperGrid::onUnbind(GtkSignalListItemFactory * /*factory*/,
                             GtkListItem * /*item*/, gpointer /*user_data*/) {
  // Optional: clear large resources if needed, but not strictly required
}

void WallpaperGrid::onTeardown(GtkSignalListItemFactory * /*factory*/,
                               GtkListItem *item, gpointer /*user_data*/) {
  GtkWidget *widget = gtk_list_item_get_child(item);
  WallpaperCard *card = static_cast<WallpaperCard *>(
      g_object_get_data(G_OBJECT(widget), "card-ptr"));

  if (card) {
    delete card;
  }
}

void WallpaperGrid::onSelectionChanged(GtkSelectionModel *model,
                                       guint /*position*/, guint /*n_items*/,
                                       gpointer user_data) {
  WallpaperGrid *self = static_cast<WallpaperGrid *>(user_data);
  if (!self->m_callback)
    return;

  // Use gtk_single_selection_get_selected_item
  gpointer item =
      gtk_single_selection_get_selected_item(GTK_SINGLE_SELECTION(model));
  if (item) {
    BwpWallpaperObject *obj = BWP_WALLPAPER_OBJECT(item);
    const bwp::wallpaper::WallpaperInfo *info =
        bwp_wallpaper_object_get_info(obj);
    if (info) {
      self->m_callback(*info);
    }
  }
}

} // namespace bwp::gui
