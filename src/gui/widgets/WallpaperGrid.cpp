#include "WallpaperGrid.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/utils/StringUtils.hpp"
#include "../../core/wallpaper/FolderManager.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "TagEditorDialog.hpp"
#include "WallpaperCard.hpp"

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

  // Initialize Tag Editor
  // Note: Parent window might not be fully ready here, but we can pass null or
  // handle it later Better: create it lazily or pass root later. AdwWindow
  // doesn't Strictly need parent. We'll traverse up to find root when needed,
  // or pass nullptr for now.
  m_tagEditor = std::make_unique<TagEditorDialog>(nullptr);

  m_tagEditor->setCallback(
      [](const std::string &id, const std::vector<std::string> &tags) {
        auto &library = bwp::wallpaper::WallpaperLibrary::getInstance();
        auto infoOpt = library.getWallpaper(id);
        if (infoOpt) {
          auto info = *infoOpt;
          info.tags = tags;
          library.updateWallpaper(info);
        }
      });
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

void WallpaperGrid::clear() {
  g_list_store_remove_all(m_store);
  m_existingPaths.clear();
}

void WallpaperGrid::addWallpaper(const bwp::wallpaper::WallpaperInfo &info) {
  // Check for duplicates using set (O(1) lookup, no store iteration)
  if (m_existingPaths.count(info.path) > 0) {
    return; // Already exists, skip
  }

  // Add to tracking set
  m_existingPaths.insert(info.path);

  // Add to store
  BwpWallpaperObject *obj = bwp_wallpaper_object_new(info);
  g_list_store_append(m_store, obj);
  g_object_unref(obj);
}

// Helper struct to pass filter state to C callback
struct FilterState {
  std::string query;
  bool favoritesOnly;
  std::string tag;
};

void WallpaperGrid::filter(const std::string &query) {
  m_filterQuery = query;
  updateFilter();
}

void WallpaperGrid::setFilterFavoritesOnly(bool onlyFavorites) {
  m_filterFavorites = onlyFavorites;
  updateFilter();
}

void WallpaperGrid::setFilterTag(const std::string &tag) {
  m_filterTag = tag;
  updateFilter();
}

void WallpaperGrid::updateFilter() {
  FilterState *state =
      new FilterState{m_filterQuery, m_filterFavorites, m_filterTag};

  auto matchFunc = [](gpointer item, gpointer user_data) -> gboolean {
    FilterState *s = static_cast<FilterState *>(user_data);
    BwpWallpaperObject *obj = BWP_WALLPAPER_OBJECT(item);
    const auto *info = bwp_wallpaper_object_get_info(obj);

    if (!info)
      return FALSE;

    // 1. Check favorites
    if (s->favoritesOnly && !info->favorite)
      return FALSE;

    // 2. Check Tag
    if (!s->tag.empty()) {
      bool tagFound = false;
      for (const auto &t : info->tags) {
        if (t == s->tag) {
          tagFound = true;
          break;
        }
      }
      if (!tagFound)
        return FALSE;
    }

    // 3. Check Query (Name, path, tags)
    if (!s->query.empty()) {
      // Re-use logic or implement simple check here
      std::string q = bwp::utils::StringUtils::toLower(s->query);
      std::string name = bwp::utils::StringUtils::toLower(
          std::filesystem::path(info->path).filename().string());

      bool match = false;
      if (name.find(q) != std::string::npos)
        match = true;

      // Search tags too
      for (const auto &t : info->tags) {
        if (bwp::utils::StringUtils::toLower(t).find(q) != std::string::npos) {
          match = true;
          break;
        }
      }
      if (!match)
        return FALSE;
    }

    return TRUE;
  };

  if (m_filter)
    g_object_unref(m_filter);
  m_filter = gtk_custom_filter_new(matchFunc, state, [](gpointer data) {
    delete static_cast<FilterState *>(data);
  });
  gtk_filter_list_model_set_filter(m_filterModel, GTK_FILTER(m_filter));
}

void WallpaperGrid::setSortOrder(int /*sortInfo*/) {
  // Todo: Implement sort sorter
}

void WallpaperGrid::setSelectionCallback(SelectionCallback callback) {
  m_callback = callback;
}

void WallpaperGrid::setReorderCallback(ReorderCallback callback) {
  m_reorderCallback = callback;
}

// Static Callbacks

void WallpaperGrid::onSetup(GtkSignalListItemFactory * /*factory*/,
                            GtkListItem *item, gpointer user_data) {
  LOG_DEBUG("onSetup");
  WallpaperGrid *self = static_cast<WallpaperGrid *>(user_data);

  // Create a WallpaperCard. Since we don't have info yet, create with dummy
  // info. But WallpaperCard expects valid info. We can construct with empty
  // info.
  bwp::wallpaper::WallpaperInfo dummy;
  WallpaperCard *card = new WallpaperCard(dummy);

  // Right Click Gesture
  // Right Click Gesture
  GtkGesture *rightClick = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(rightClick), 3);

  // Drag Source
  GtkDragSource *dragSource = gtk_drag_source_new();
  g_signal_connect(dragSource, "prepare",
                   G_CALLBACK(+[](GtkDragSource *source, double, double,
                                  gpointer) -> GdkContentProvider * {
                     WallpaperGrid *grid = static_cast<WallpaperGrid *>(
                         g_object_get_data(G_OBJECT(source), "grid"));
                     GtkWidget *widget = gtk_event_controller_get_widget(
                         GTK_EVENT_CONTROLLER(source));
                     WallpaperCard *card = static_cast<WallpaperCard *>(
                         g_object_get_data(G_OBJECT(widget), "card-ptr"));

                     if (card) {
                       std::string id = card->getInfo().id;
                       return gdk_content_provider_new_for_bytes(
                           "application/x-bwp-wallpaper-id",
                           g_bytes_new(id.c_str(), id.length() + 1));
                     }
                     return nullptr;
                   }),
                   nullptr);

  // Attach grid pointer to source
  g_object_set_data(G_OBJECT(dragSource), "grid", self);
  gtk_widget_add_controller(card->getWidget(),
                            GTK_EVENT_CONTROLLER(dragSource));

  // Drop Target
  GtkDropTarget *dropTarget =
      gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_MOVE);
  GType types[] = {G_TYPE_BYTES};
  gtk_drop_target_set_gtypes(dropTarget, types, 1);

  g_signal_connect(
      dropTarget, "drop",
      G_CALLBACK(+[](GtkDropTarget *target, const GValue *value, double, double,
                     gpointer user_data) -> gboolean {
        WallpaperGrid *grid = static_cast<WallpaperGrid *>(user_data);
        GtkWidget *widget =
            gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
        WallpaperCard *card = static_cast<WallpaperCard *>(
            g_object_get_data(G_OBJECT(widget), "card-ptr"));

        if (G_VALUE_HOLDS(value, G_TYPE_BYTES) && card &&
            grid->m_reorderCallback) {
          GBytes *bytes = (GBytes *)g_value_get_boxed(value);
          if (bytes) {
            gsize len;
            const char *data = (const char *)g_bytes_get_data(bytes, &len);
            std::string sourceId(data);
            std::string targetId = card->getInfo().id;

            if (sourceId != targetId) {
              // Determine if dropping left (before) or right (after)?
              // For simplicity, let's say "dropping onto means insert before"
              // unless we can detect coordinates. Ideally we check x coordinate
              // relative to width/2. But for now, let's just assume "insert
              // before" or verify if user wants "behind or in front". User said
              // "behind or in front". Let's settle on: Default before.
              grid->m_reorderCallback(sourceId, targetId, false);
              return TRUE;
            }
          }
        }
        return FALSE;
      }),
      self);

  gtk_widget_add_controller(card->getWidget(),
                            GTK_EVENT_CONTROLLER(dropTarget));

  auto rightClickCallback = +[](GtkGestureClick *gesture, int /*n_press*/,
                                double x, double y, gpointer /*data*/) {
    // Handle right click
    GtkWidget *widget =
        gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    if (!widget || !G_IS_OBJECT(widget))
      return;

    WallpaperCard *card = static_cast<WallpaperCard *>(
        g_object_get_data(G_OBJECT(widget), "card-ptr"));
    WallpaperGrid *grid = static_cast<WallpaperGrid *>(
        g_object_get_data(G_OBJECT(widget), "grid-ptr"));

    if (card && grid) {
      const auto &info = card->getInfo();

      // Show Menu
      GtkWidget *popover = gtk_popover_new();
      gtk_widget_set_parent(popover, widget);

      GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_popover_set_child(GTK_POPOVER(popover), box);

      auto addButton = [&](const char *label, const char *icon) {
        GtkWidget *btn = gtk_button_new_from_icon_name(icon);
        GtkWidget *lb = gtk_label_new(label);

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(row), gtk_image_new_from_icon_name(icon));
        gtk_box_append(GTK_BOX(row), lb);

        GtkWidget *button = gtk_button_new();
        gtk_button_set_child(GTK_BUTTON(button), row);
        gtk_widget_add_css_class(button, "flat");

        // Pass copy of ID to ensure valid reference
        std::string *idPtr = new std::string(info.id);
        std::vector<std::string> *tagsPtr =
            new std::vector<std::string>(info.tags);

        g_object_set_data_full(
            G_OBJECT(button), "wp_id", idPtr,
            [](gpointer d) { delete static_cast<std::string *>(d); });
        g_object_set_data_full(
            G_OBJECT(button), "wp_tags", tagsPtr, [](gpointer d) {
              delete static_cast<std::vector<std::string> *>(d);
            });

        g_signal_connect(
            button, "clicked", G_CALLBACK(+[](GtkButton *b, gpointer d) {
              auto *g = static_cast<WallpaperGrid *>(d);
              std::string *id = static_cast<std::string *>(
                  g_object_get_data(G_OBJECT(b), "wp_id"));
              std::vector<std::string> *tags =
                  static_cast<std::vector<std::string> *>(
                      g_object_get_data(G_OBJECT(b), "wp_tags"));

              if (id && tags && g->m_tagEditor) {
                g->m_tagEditor->show(*id, *tags);
              }

              GtkWidget *pop =
                  gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
              if (pop)
                gtk_popover_popdown(GTK_POPOVER(pop));
            }),
            grid);

        gtk_box_append(GTK_BOX(box), button);
      };

      addButton("Edit Tags", "tag-symbolic");

      // Add to Folder section
      auto folders = bwp::wallpaper::FolderManager::getInstance().getFolders();
      if (!folders.empty()) {
        gtk_box_append(GTK_BOX(box),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

        GtkWidget *label = gtk_label_new("Add to Folder:");
        gtk_widget_add_css_class(label, "caption");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(label, 12);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_box_append(GTK_BOX(box), label);

        for (const auto &folder : folders) {
          GtkWidget *btn = gtk_button_new();
          GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
          gtk_box_append(GTK_BOX(row),
                         gtk_image_new_from_icon_name(folder.icon.c_str()));
          gtk_box_append(GTK_BOX(row), gtk_label_new(folder.name.c_str()));
          gtk_button_set_child(GTK_BUTTON(btn), row);
          gtk_widget_add_css_class(btn, "flat");

          // Pass IDs
          std::string *wpIdPtr = new std::string(info.id);
          std::string *folderIdPtr = new std::string(folder.id);

          g_object_set_data_full(
              G_OBJECT(btn), "wp_id", wpIdPtr,
              [](gpointer d) { delete static_cast<std::string *>(d); });
          g_object_set_data_full(
              G_OBJECT(btn), "folder_id", folderIdPtr,
              [](gpointer d) { delete static_cast<std::string *>(d); });

          g_signal_connect(
              btn, "clicked", G_CALLBACK(+[](GtkButton *b, gpointer d) {
                std::string *wpId = static_cast<std::string *>(
                    g_object_get_data(G_OBJECT(b), "wp_id"));
                std::string *folderId = static_cast<std::string *>(
                    g_object_get_data(G_OBJECT(b), "folder_id"));

                if (wpId && folderId) {
                  bwp::wallpaper::FolderManager::getInstance().addToFolder(
                      *folderId, *wpId);
                  LOG_INFO("Added wallpaper " + *wpId + " to folder " +
                           *folderId);
                }

                GtkWidget *pop =
                    gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
                if (pop)
                  gtk_popover_popdown(GTK_POPOVER(pop));
              }),
              nullptr);

          gtk_box_append(GTK_BOX(box), btn);
        }
      }

      gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);

      // Position at click
      GdkRectangle rect = {(int)x, (int)y, 1, 1};
      gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);

      gtk_popover_popup(GTK_POPOVER(popover));
    }
  };

  g_signal_connect(rightClick, "pressed", G_CALLBACK(rightClickCallback),
                   nullptr);
  gtk_widget_add_controller(card->getWidget(),
                            GTK_EVENT_CONTROLLER(rightClick));

  // Center alignment for the grid cell
  gtk_widget_set_halign(card->getWidget(), GTK_ALIGN_CENTER);
  gtk_widget_set_valign(card->getWidget(), GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(card->getWidget(), 8);
  gtk_widget_set_margin_end(card->getWidget(), 8);
  gtk_widget_set_margin_top(card->getWidget(), 8);
  gtk_widget_set_margin_bottom(card->getWidget(), 8);

  // Attach card pointer to widget so we can retrieve it
  g_object_set_data(G_OBJECT(card->getWidget()), "card-ptr", card);
  g_object_set_data(G_OBJECT(card->getWidget()), "grid-ptr", self);

  gtk_list_item_set_child(item, card->getWidget());
}

void WallpaperGrid::onBind(GtkSignalListItemFactory * /*factory*/,
                           GtkListItem *item, gpointer /*user_data*/) {
  GtkWidget *widget = gtk_list_item_get_child(item);
  if (!widget || !G_IS_OBJECT(widget)) {
    LOG_WARN("onBind: widget is null or invalid");
    return;
  }

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
  if (!widget || !G_IS_OBJECT(widget)) {
    return;
  }

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
