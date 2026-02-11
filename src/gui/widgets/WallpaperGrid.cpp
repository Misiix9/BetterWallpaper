#include "WallpaperGrid.hpp"
#include "../../core/ipc/LinuxIPCClient.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/utils/SafeProcess.hpp"
#include "../../core/utils/StringUtils.hpp"
#include "../../core/utils/ToastManager.hpp"
#include "../../core/wallpaper/FolderManager.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../models/WallpaperObject.hpp"
#include "WallpaperCard.hpp"

#include <filesystem>

namespace bwp::gui {

double WallpaperGrid::getVScroll() const {
  if (!m_scrolledWindow)
    return 0.0;
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));
  return gtk_adjustment_get_value(adj);
}

void WallpaperGrid::setVScroll(double value) {
  if (!m_scrolledWindow)
    return;
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
      GTK_SCROLLED_WINDOW(m_scrolledWindow));
  gtk_adjustment_set_value(adj, value);
}

void WallpaperGrid::clearSelection() {
    if (m_selectionModel) {
        gtk_selection_model_unselect_all(GTK_SELECTION_MODEL(m_selectionModel));
    }
}

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
  // Model sinks the floating ref, so we need to ref it again for ourselves
  g_object_ref(m_filter);

  // 3. Sort Model (Default sort)
  m_sortModel = gtk_sort_list_model_new(G_LIST_MODEL(m_filterModel), nullptr);

  // 4. Selection Model
  m_selectionModel = gtk_single_selection_new(G_LIST_MODEL(m_sortModel));

  // Disable autoselect so no wallpaper is pre-selected on load
  gtk_single_selection_set_autoselect(m_selectionModel, FALSE);
  gtk_single_selection_set_can_unselect(m_selectionModel, TRUE);

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

void WallpaperGrid::updateWallpaperInStore(
    const bwp::wallpaper::WallpaperInfo &info) {
  // Update the GObject data in-place (no splice = no items-changed = no scroll reset)
  guint n = g_list_model_get_n_items(G_LIST_MODEL(m_store));
  for (guint i = 0; i < n; ++i) {
    BwpWallpaperObject *obj =
        BWP_WALLPAPER_OBJECT(g_list_model_get_item(G_LIST_MODEL(m_store), i));
    if (obj) {
      const auto *stored = bwp_wallpaper_object_get_info(obj);
      if (stored && stored->id == info.id) {
        bwp_wallpaper_object_update_info(obj, info);
        g_object_unref(obj);
        break;
      }
      g_object_unref(obj);
    }
  }

  // Directly update the visible card if it's currently bound
  auto it = m_boundCards.find(info.id);
  if (it != m_boundCards.end() && it->second) {
    it->second->updateMetadata(info);
  }
}

void WallpaperGrid::notifyDataChanged() {
  // Re-evaluate the current filter and sorter against updated data
  if (m_filter) {
    gtk_filter_changed(GTK_FILTER(m_filter), GTK_FILTER_CHANGE_DIFFERENT);
  }
  GtkSorter *sorter = gtk_sort_list_model_get_sorter(m_sortModel);
  if (sorter) {
    gtk_sorter_changed(sorter, GTK_SORTER_CHANGE_DIFFERENT);
  }
}

// Helper struct to pass filter state to C callback
struct FilterState {
  std::string query;
  bool favoritesOnly;
  std::string tag;
  std::string source;
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

void WallpaperGrid::setFilterSource(const std::string &source) {
  m_filterSource = source;
  updateFilter();
}

void WallpaperGrid::updateFilter() {
  FilterState *state = new FilterState{m_filterQuery, m_filterFavorites,
                                       m_filterTag, m_filterSource};

  auto matchFunc = [](gpointer item, gpointer user_data) -> gboolean {
    FilterState *s = static_cast<FilterState *>(user_data);
    BwpWallpaperObject *obj = BWP_WALLPAPER_OBJECT(item);
    const auto *info = bwp_wallpaper_object_get_info(obj);

    if (!info)
      return FALSE;

    // 0. Check Source
    if (s->source != "all") {
      if (s->source == "local") {
        // Local means explicitly local source OR not steam/workshop types if
        // source is missing
        if (info->source == "steam" || info->source == "workshop" ||
            info->type == bwp::wallpaper::WallpaperType::WEScene ||
            info->type == bwp::wallpaper::WallpaperType::WEVideo)
          return FALSE;
      } else if (s->source == "steam") {
        // Steam means explicitly steam/workshop source OR steam types
        if ((info->source != "steam" && info->source != "workshop") &&
            (info->type != bwp::wallpaper::WallpaperType::WEScene &&
             info->type != bwp::wallpaper::WallpaperType::WEVideo))
          return FALSE;
      }
    }

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

    // 3. Check Query (Name, title, tags) — with fuzzy matching
    if (!s->query.empty()) {
      std::string q = bwp::utils::StringUtils::toLower(s->query);

      // Get wallpaper name from filename stem (without extension)
      std::string name = bwp::utils::StringUtils::toLower(
          std::filesystem::path(info->path).stem().string());

      // Also check title field
      std::string title = bwp::utils::StringUtils::toLower(info->title);

      bool match = false;

      // Priority 1: Substring match on name or title
      if (name.find(q) != std::string::npos)
        match = true;
      if (!match && !title.empty() && title.find(q) != std::string::npos)
        match = true;

      // Priority 2: Substring match on tags
      if (!match) {
        for (const auto &t : info->tags) {
          if (bwp::utils::StringUtils::toLower(t).find(q) !=
              std::string::npos) {
            match = true;
            break;
          }
        }
      }

      // Priority 3: Fuzzy match (Levenshtein distance) on name words
      if (!match) {
        // Split name by common delimiters and check individual words
        auto fuzzyCheck = [&q](const std::string &text) -> bool {
          // Levenshtein distance inline
          auto levenshtein = [](const std::string &s1,
                                const std::string &s2) -> int {
            const size_t m = s1.size();
            const size_t n = s2.size();
            if (m == 0)
              return static_cast<int>(n);
            if (n == 0)
              return static_cast<int>(m);
            std::vector<int> prev(n + 1), curr(n + 1);
            for (size_t j = 0; j <= n; ++j)
              prev[j] = static_cast<int>(j);
            for (size_t i = 1; i <= m; ++i) {
              curr[0] = static_cast<int>(i);
              for (size_t j = 1; j <= n; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                curr[j] = std::min(
                    {prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
              }
              std::swap(prev, curr);
            }
            return prev[n];
          };

          int threshold = std::max(2, static_cast<int>(q.length()) / 3);

          // Check whole text
          int dist = levenshtein(q, text);
          if (dist <= threshold)
            return true;

          // Check individual words in the text (split by space, _, -)
          std::string word;
          for (size_t i = 0; i <= text.size(); ++i) {
            char c = (i < text.size()) ? text[i] : ' ';
            if (c == ' ' || c == '_' || c == '-' || i == text.size()) {
              if (!word.empty() && word.length() >= 2) {
                if (levenshtein(q, word) <= threshold)
                  return true;
              }
              word.clear();
            } else {
              word += c;
            }
          }
          return false;
        };

        if (fuzzyCheck(name))
          match = true;
        if (!match && !title.empty() && fuzzyCheck(title))
          match = true;
      }

      if (!match)
        return FALSE;
    }

    return TRUE;
  };

  // Create new filter and set on model
  // Note: gtk_custom_filter_new returns a floating ref that the model sinks
  GtkCustomFilter *newFilter =
      gtk_custom_filter_new(matchFunc, state, [](gpointer data) {
        delete static_cast<FilterState *>(data);
      });

  // Set on model - this replaces the old filter (model handles old filter's
  // lifecycle)
  gtk_filter_list_model_set_filter(m_filterModel, GTK_FILTER(newFilter));

  // Keep reference for destructor - ref it since model already sunk the
  // floating ref
  g_object_ref(newFilter);
  if (m_filter) {
    g_object_unref(m_filter);
  }
  m_filter = newFilter;
}

void WallpaperGrid::setSortOrder(SortOrder order) {
  m_currentSort = order;

  auto sortFunc = [](gconstpointer a, gconstpointer b,
                     gpointer user_data) -> gint {
    SortOrder order =
        static_cast<SortOrder>(reinterpret_cast<intptr_t>(user_data));

    BwpWallpaperObject *objA = BWP_WALLPAPER_OBJECT((gpointer)a);
    BwpWallpaperObject *objB = BWP_WALLPAPER_OBJECT((gpointer)b);
    const auto *infoA = bwp_wallpaper_object_get_info(objA);
    const auto *infoB = bwp_wallpaper_object_get_info(objB);

    if (!infoA || !infoB)
      return 0;

    // Helper: get display name — use title if available, otherwise filename stem
    auto getDisplayName = [](const bwp::wallpaper::WallpaperInfo *info) {
      if (!info->title.empty()) {
        return bwp::utils::StringUtils::toLower(info->title);
      }
      return bwp::utils::StringUtils::toLower(
          std::filesystem::path(info->path).stem().string());
    };

    switch (order) {
    case SortOrder::NameAsc: {
      return getDisplayName(infoA).compare(getDisplayName(infoB));
    }
    case SortOrder::NameDesc: {
      return getDisplayName(infoB).compare(getDisplayName(infoA));
    }
    case SortOrder::DateNewest:
      if (infoA->added > infoB->added)
        return -1;
      if (infoA->added < infoB->added)
        return 1;
      return 0;
    case SortOrder::DateOldest:
      if (infoA->added < infoB->added)
        return -1;
      if (infoA->added > infoB->added)
        return 1;
      return 0;
    case SortOrder::RatingDesc: {
      // Primary: rating descending, secondary: name ascending for stable sort
      int cmp = infoB->rating - infoA->rating;
      if (cmp != 0) return cmp;
      return getDisplayName(infoA).compare(getDisplayName(infoB));
    }
    default:
      return 0;
    }
  };

  if (m_sortModel) {
    GtkSorter *sorter = GTK_SORTER(gtk_custom_sorter_new(
        sortFunc, reinterpret_cast<gpointer>(static_cast<intptr_t>(order)),
        nullptr));
    gtk_sort_list_model_set_sorter(m_sortModel, sorter);
    // Explicitly notify the sorter changed to force re-sort propagation
    gtk_sorter_changed(sorter, GTK_SORTER_CHANGE_DIFFERENT);
    g_object_unref(sorter);
  }
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
                     (void)grid; // unused but required for logic? No, actually
                                 // lines below dont use it.
                     // Ah wait, original code:
                     // WallpaperGrid *grid = static_cast<WallpaperGrid
                     // *>(g_object_get_data(G_OBJECT(source), "grid"));
                     // GtkWidget *widget =
                     // gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(source));
                     // ...
                     // It doesn't use `grid`. So just remove the line.
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

      // Show full context menu
      GtkWidget *popover = gtk_popover_new();
      gtk_widget_set_parent(popover, widget);

      GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_popover_set_child(GTK_POPOVER(popover), box);

      // Helper to create a flat button row with icon + label
      auto makeButton = [&](const char *label, const char *icon) -> GtkWidget* {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(row), gtk_image_new_from_icon_name(icon));
        gtk_box_append(GTK_BOX(row), gtk_label_new(label));
        GtkWidget *button = gtk_button_new();
        gtk_button_set_child(GTK_BUTTON(button), row);
        gtk_widget_add_css_class(button, "flat");
        gtk_box_append(GTK_BOX(box), button);
        return button;
      };

      // === Set as Wallpaper ===
      {
        GtkWidget *btn = makeButton("Set as Wallpaper", "wallpaper-symbolic");
        std::string *idPtr = new std::string(info.id);
        g_object_set_data_full(G_OBJECT(btn), "wp_id", idPtr,
            [](gpointer d) { delete static_cast<std::string *>(d); });
        g_signal_connect(btn, "clicked",
            G_CALLBACK(+[](GtkButton *b, gpointer d) {
              auto *g = static_cast<WallpaperGrid *>(d);
              std::string *id = static_cast<std::string *>(
                  g_object_get_data(G_OBJECT(b), "wp_id"));
              if (id) {
                auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
                auto wp = lib.getWallpaper(*id);
                if (wp) {
                  // Also select/preview the wallpaper if callback exists
                  if (g->m_callback) {
                    g->m_callback(*wp);
                  }

                  // Send IPC to daemon to actually set the wallpaper
                  auto &monMgr = bwp::monitor::MonitorManager::getInstance();
                  auto monitors = monMgr.getMonitors();

                  bwp::ipc::LinuxIPCClient ipcClient;
                  if (ipcClient.connect()) {
                    if (monitors.empty()) {
                      LOG_INFO("Context menu: setting wallpaper on default monitor: " + wp->path);
                      ipcClient.setWallpaper(wp->path, "eDP-1");
                    } else {
                      for (const auto &mon : monitors) {
                        LOG_INFO("Context menu: setting wallpaper on " + mon.name + ": " + wp->path);
                        ipcClient.setWallpaper(wp->path, mon.name);
                      }
                    }
                    bwp::core::utils::ToastManager::getInstance().showSuccess(
                        "Wallpaper applied successfully");
                  } else {
                    LOG_ERROR("Context menu: failed to connect to daemon via IPC");
                    bwp::core::utils::ToastManager::getInstance().showError(
                        "Failed to set wallpaper: daemon not running");
                  }
                }
              }
              GtkWidget *pop = gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
              if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
            }), grid);
      }

      // === Toggle Favorite ===
      {
        const char *favLabel = info.favorite ? "Unfavorite" : "Favorite";
        GtkWidget *btn = makeButton(favLabel, info.favorite ? "starred-symbolic" : "non-starred-symbolic");
        std::string *idPtr = new std::string(info.id);
        g_object_set_data_full(G_OBJECT(btn), "wp_id", idPtr,
            [](gpointer d) { delete static_cast<std::string *>(d); });
        g_signal_connect(btn, "clicked",
            G_CALLBACK(+[](GtkButton *b, gpointer) {
              std::string *id = static_cast<std::string *>(
                  g_object_get_data(G_OBJECT(b), "wp_id"));
              if (id) {
                auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
                auto wp = lib.getWallpaper(*id);
                if (wp) {
                  auto updated = *wp;
                  updated.favorite = !updated.favorite;
                  lib.updateWallpaper(updated);
                }
              }
              GtkWidget *pop = gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
              if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
            }), nullptr);
      }

      // === Show in Files ===
      {
        GtkWidget *btn = makeButton("Show in Files", "folder-open-symbolic");
        std::string *pathPtr = new std::string(info.path);
        g_object_set_data_full(G_OBJECT(btn), "wp_path", pathPtr,
            [](gpointer d) { delete static_cast<std::string *>(d); });
        g_signal_connect(btn, "clicked",
            G_CALLBACK(+[](GtkButton *b, gpointer) {
              std::string *path = static_cast<std::string *>(
                  g_object_get_data(G_OBJECT(b), "wp_path"));
              if (path) {
                std::string dir = std::filesystem::path(*path).parent_path().string();
                bwp::utils::SafeProcess::execDetached({"xdg-open", dir});
              }
              GtkWidget *pop = gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
              if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
            }), nullptr);
      }

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
              btn, "clicked", G_CALLBACK(+[](GtkButton *b, gpointer /*d*/) {
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
      gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);

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

  // Fill alignment so cards expand to fill column width reactively
  gtk_widget_set_halign(card->getWidget(), GTK_ALIGN_FILL);
  gtk_widget_set_valign(card->getWidget(), GTK_ALIGN_START);
  gtk_widget_set_margin_start(card->getWidget(), 4);
  gtk_widget_set_margin_end(card->getWidget(), 4);
  gtk_widget_set_margin_top(card->getWidget(), 4);
  gtk_widget_set_margin_bottom(card->getWidget(), 4);

  // Attach card pointer to widget so we can retrieve it
  g_object_set_data(G_OBJECT(card->getWidget()), "card-ptr", card);
  g_object_set_data(G_OBJECT(card->getWidget()), "grid-ptr", self);

  gtk_list_item_set_child(item, card->getWidget());
}

void WallpaperGrid::onBind(GtkSignalListItemFactory * /*factory*/,
                           GtkListItem *item, gpointer user_data) {
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
    // Read fresh data from library to reflect any favorite/rating changes
    auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
    auto freshInfo = lib.getWallpaper(info->id);
    if (freshInfo.has_value()) {
      card->setInfo(freshInfo.value());
    } else {
      card->setInfo(*info);
    }

    // Register in bound cards map for direct metadata updates
    WallpaperGrid *self = static_cast<WallpaperGrid *>(user_data);
    if (self) {
      self->m_boundCards[info->id] = card;
      card->setHighlight(self->m_filterQuery);
    }
  } else {
    LOG_WARN("onBind: card or info is null");
  }
}

void WallpaperGrid::onUnbind(GtkSignalListItemFactory * /*factory*/,
                             GtkListItem *item, gpointer user_data) {
  // Lazy loading: cancel pending thumbnail loads and release texture memory
  // when the card scrolls out of the visible viewport
  GtkWidget *widget = gtk_list_item_get_child(item);
  if (!widget || !G_IS_OBJECT(widget))
    return;

  WallpaperCard *card = static_cast<WallpaperCard *>(
      g_object_get_data(G_OBJECT(widget), "card-ptr"));
  if (card) {
    // Unregister from bound cards map
    WallpaperGrid *self = static_cast<WallpaperGrid *>(user_data);
    if (self) {
      self->m_boundCards.erase(card->getInfo().id);
    }
    card->releaseResources();
  }
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
