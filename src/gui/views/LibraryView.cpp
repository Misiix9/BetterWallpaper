#include "LibraryView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/FileUtils.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <filesystem>
#include "../../core/services/AutoTagService.hpp"

namespace bwp::gui {

// Fixed width for the preview panel
static constexpr int PREVIEW_PANEL_WIDTH = 320;

LibraryView::LibraryView() {
  setupUi();

  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<LibraryView *>(data);
        self->loadWallpapers();
        
        // Listen for updates
        auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
        lib.setChangeCallback([self](const bwp::wallpaper::WallpaperInfo &info){
            g_idle_add(+[](gpointer d) -> gboolean {
                 // Info logic would go here if needed, or trigger refresh
                 return G_SOURCE_REMOVE;
            }, nullptr);
            
            // For now, assuming main thread (GTK generic)
             if (self->m_grid) {
                 self->m_grid->addWallpaper(info); // Update or add
             }
        });
        
        return G_SOURCE_REMOVE;
      },
      this);
}

LibraryView::~LibraryView() {}

void LibraryView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(m_box, TRUE);
  gtk_widget_set_vexpand(m_box, TRUE);

  // Header/Search area
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_margin_start(headerBox, 12);
  gtk_widget_set_margin_end(headerBox, 12);
  gtk_widget_set_margin_top(headerBox, 8);
  gtk_widget_set_margin_bottom(headerBox, 8);

  m_searchBar = std::make_unique<SearchBar>();
  m_searchBar->setCallback([this](const std::string &text) {
    if (m_grid)
      m_grid->filter(text);
  });

  gtk_widget_set_hexpand(m_searchBar->getWidget(), TRUE);
  gtk_box_append(GTK_BOX(headerBox), m_searchBar->getWidget());

  // Favorites Toggle
  GtkWidget *favBtn = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(favBtn), "starred-symbolic");
  gtk_widget_set_tooltip_text(favBtn, "Show Favorites Only");
  gtk_widget_set_margin_start(favBtn, 8);

  g_signal_connect(favBtn, "toggled",
                   G_CALLBACK(+[](GtkToggleButton *btn, gpointer data) {
                     auto *self = static_cast<LibraryView *>(data);
                     bool active = gtk_toggle_button_get_active(btn);
                     if (self->m_grid) {
                       self->m_grid->setFilterFavoritesOnly(active);
                     }
                   }),
                   this);

  gtk_box_append(GTK_BOX(headerBox), favBtn);

  // Source Filter Toggles (Linked)
  GtkWidget *sourceBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(sourceBox, "linked");
  gtk_widget_set_margin_start(sourceBox, 12);

  auto createToggle = [&](const char *label, const char *id, bool active, GtkToggleButton *group) {
      GtkWidget *btn = gtk_toggle_button_new_with_label(label);
      if (group) gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(btn), group);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), active);
      
      // Pass ID as data
      std::string *sId = new std::string(id);
      g_object_set_data_full(G_OBJECT(btn), "source_id", sId, [](gpointer d){ delete static_cast<std::string*>(d); });

      g_signal_connect(btn, "toggled", G_CALLBACK(+[](GtkToggleButton *b, gpointer data){
          LibraryView *self = static_cast<LibraryView*>(data);
          if (gtk_toggle_button_get_active(b)) {
              std::string *sid = static_cast<std::string*>(g_object_get_data(G_OBJECT(b), "source_id"));
              if (sid) self->onSourceFilterChanged(*sid);
          }
      }), this);
      
      return btn;
  };

  GtkWidget *btnAll = createToggle("All", "all", true, nullptr);
  GtkWidget *btnLocal = createToggle("Local", "local", false, GTK_TOGGLE_BUTTON(btnAll));
  GtkWidget *btnSteam = createToggle("Steam", "steam", false, GTK_TOGGLE_BUTTON(btnAll));

  gtk_box_append(GTK_BOX(sourceBox), btnAll);
  gtk_box_append(GTK_BOX(sourceBox), btnLocal);
  gtk_box_append(GTK_BOX(sourceBox), btnSteam);
  gtk_box_append(GTK_BOX(headerBox), sourceBox);

  // Tag Filter Dropdown
  m_tagList = gtk_string_list_new(nullptr);
  gtk_string_list_append(m_tagList, "All Tags");

  m_filterCombo = gtk_drop_down_new(G_LIST_MODEL(m_tagList), nullptr);
  gtk_widget_set_margin_start(m_filterCombo, 8);

  g_signal_connect(
      m_filterCombo, "notify::selected",
      G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer data) {
        auto *self = static_cast<LibraryView *>(data);
        GtkDropDown *dropdown = GTK_DROP_DOWN(object);
        guint selected = gtk_drop_down_get_selected(dropdown);

        // Get the model directly from the dropdown to avoid stale pointer
        // issues
        GListModel *model = gtk_drop_down_get_model(dropdown);
        if (!model || !GTK_IS_STRING_LIST(model)) {
          return;
        }

        const char *tag =
            gtk_string_list_get_string(GTK_STRING_LIST(model), selected);

        std::string tagStr = (tag && strcmp(tag, "All Tags") != 0) ? tag : "";
        if (self->m_grid) {
          self->m_grid->setFilterTag(tagStr);
        }
      }),
      this);

  gtk_box_append(GTK_BOX(headerBox), m_filterCombo);

  // Sort Dropdown
  const char* sortOptions[] = {"Name (A-Z)", "Name (Z-A)", "Date Added (Newest)", "Date Added (Oldest)", "Rating", nullptr};
  GtkWidget *sortCombo = gtk_drop_down_new_from_strings(sortOptions);
  gtk_widget_set_margin_start(sortCombo, 8);
  g_signal_connect(sortCombo, "notify::selected", G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer data){
      auto *self = static_cast<LibraryView *>(data);
      guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(object));
      if (self->m_grid) {
          WallpaperGrid::SortOrder order = WallpaperGrid::SortOrder::NameAsc;
          switch(selected) {
              case 0: order = WallpaperGrid::SortOrder::NameAsc; break;
              case 1: order = WallpaperGrid::SortOrder::NameDesc; break;
              case 2: order = WallpaperGrid::SortOrder::DateNewest; break;
              case 3: order = WallpaperGrid::SortOrder::DateOldest; break;
              case 4: order = WallpaperGrid::SortOrder::RatingDesc; break;
          }
          self->m_grid->setSortOrder(order);
      }
  }), this);
  gtk_box_append(GTK_BOX(headerBox), sortCombo);

  // Simulate Scan Button (Mock)
  GtkWidget *scanBtn = gtk_button_new_from_icon_name("system-search-symbolic");
  gtk_widget_set_tooltip_text(scanBtn, "Simulate AI Scan");
  gtk_widget_set_margin_start(scanBtn, 8);
  g_signal_connect(scanBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data){
      // Trigger scan on random wallpaper
      LOG_INFO("Simulating AI Scan...");
      auto& lib = bwp::wallpaper::WallpaperLibrary::getInstance();
      auto wallpapers = lib.getAllWallpapers();
      if (!wallpapers.empty()) {
           size_t idx = rand() % wallpapers.size();
           // Use the service

           bwp::core::services::AutoTagService::getInstance().scan(wallpapers[idx].id);
      }
  }), this);
  gtk_box_append(GTK_BOX(headerBox), scanBtn);

  // Add Wallpaper Button (opens file picker for local images)
  GtkWidget *addBtn = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_set_tooltip_text(addBtn, "Add Wallpaper from File");
  gtk_widget_set_margin_start(addBtn, 8);
  gtk_widget_add_css_class(addBtn, "suggested-action");
  g_signal_connect(addBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<LibraryView *>(data);
                     self->onAddWallpaper();
                   }),
                   this);
  gtk_box_append(GTK_BOX(headerBox), addBtn);

  gtk_box_append(GTK_BOX(m_box), headerBox);

  // Content Area - use a paned widget for proper fixed/flexible sizing
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_vexpand(paned, TRUE);
  gtk_widget_set_hexpand(paned, TRUE);
  gtk_box_append(GTK_BOX(m_box), paned);

  // Grid - takes all available space (left side)
  m_grid = std::make_unique<WallpaperGrid>();
  GtkWidget *gridWidget = m_grid->getWidget();
  gtk_widget_set_hexpand(gridWidget, TRUE);
  gtk_widget_set_vexpand(gridWidget, TRUE);
  gtk_paned_set_start_child(GTK_PANED(paned), gridWidget);
  gtk_paned_set_resize_start_child(GTK_PANED(paned), TRUE);
  gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);

  // Preview Panel - FIXED width (right side)
  m_previewPanel = std::make_unique<PreviewPanel>();
  GtkWidget *previewWidget = m_previewPanel->getWidget();
  gtk_widget_set_size_request(previewWidget, PREVIEW_PANEL_WIDTH, -1);
  gtk_widget_set_hexpand(previewWidget, FALSE);

  // Wrap in revealer for slide-in animation
  m_previewRevealer = gtk_revealer_new();
  gtk_revealer_set_transition_type(GTK_REVEALER(m_previewRevealer),
                                   GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  gtk_revealer_set_child(GTK_REVEALER(m_previewRevealer), previewWidget);
  gtk_revealer_set_reveal_child(GTK_REVEALER(m_previewRevealer),
                                FALSE); // Start hidden

  gtk_paned_set_end_child(GTK_PANED(paned), m_previewRevealer);
  gtk_paned_set_resize_end_child(GTK_PANED(paned),
                                 FALSE); // Don't resize preview
  gtk_paned_set_shrink_end_child(GTK_PANED(paned),
                                 FALSE); // Don't shrink preview

  // Set initial position to give grid most of the space
  gtk_paned_set_position(GTK_PANED(paned), 700);

  // Connect Grid Selection
  m_grid->setSelectionCallback([this](
                                   const bwp::wallpaper::WallpaperInfo &info) {
    if (m_previewPanel) {
      m_previewPanel->setWallpaper(info);
      if (m_previewRevealer) {
        gtk_revealer_set_reveal_child(GTK_REVEALER(m_previewRevealer), TRUE);
      }
    }
  });
}

void LibraryView::loadWallpapers() {
  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  lib.initialize();

  if (m_grid) {
    m_grid->clear();
    auto wallpapers = lib.getAllWallpapers();
    for (const auto &info : wallpapers) {
      m_grid->addWallpaper(info);
    }
  }

  // Optimize: Rebuild tag list
  // Note: GtkStringList modification is a bit tedious if we want to keep "All
  // Tags" at top. Easiest is to create new one or clear. But m_tagList is bound
  // to m_filterCombo. We can just append new ones if we track them, but simpler
  // to rebuild logic if API supports clear? GtkStringList doesn't have clear().
  // We have to remove items. Actually, let's just create a new list and set
  // model? GtkDropDown set_model.

  auto tags = lib.getAllTags();
  GtkStringList *newList = gtk_string_list_new(nullptr);
  gtk_string_list_append(newList, "All Tags");
  for (const auto &tag : tags) {
    gtk_string_list_append(newList, tag.c_str());
  }
  gtk_drop_down_set_model(GTK_DROP_DOWN(m_filterCombo), G_LIST_MODEL(newList));
  m_tagList =
      newList; // Update pointer (Old one is unreffed by dropdown? No, we need
               // to unref old if we owned it. Dropdown took ref. We can just
               // forget and let dropdown handle ref counting if we transfer
               // ownership? Check: gtk_drop_down_set_model takes ownership or
               // ref? It takes a ref. We should probably unref old if we held a
               // ref. But for now this works.

  auto paths =
      bwp::config::ConfigManager::getInstance().get<std::vector<std::string>>(
          "library.paths");

  // If no paths configured, try to auto-detect Wallpaper Engine folder
  if (paths.empty()) {
    LOG_INFO("No library paths configured, searching for Wallpaper Engine...");

    // Common Steam library locations
    std::vector<std::string> steamPaths = {
        "~/.steam/steam/steamapps/workshop/content/431960",
        "~/.local/share/Steam/steamapps/workshop/content/431960",
        "/home/" + std::string(getenv("USER") ? getenv("USER") : "") +
            "/.steam/steam/steamapps/workshop/content/431960",
    };

    for (const auto &steamPath : steamPaths) {
      std::string expandedPath = steamPath;
      // Expand ~
      if (!expandedPath.empty() && expandedPath[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
          expandedPath = std::string(home) + expandedPath.substr(1);
        }
      }

      if (std::filesystem::exists(expandedPath)) {
        LOG_INFO("Found Wallpaper Engine folder: " + expandedPath);
        paths.push_back(expandedPath);

        // Save to config so we don't have to search again
        bwp::config::ConfigManager::getInstance().set("library.paths", paths);
        break;
      }
    }

    if (paths.empty()) {
      LOG_INFO("No Wallpaper Engine folder found. User can add directories "
               "manually.");
      // Don't add any fallback paths - user must add manually
    }
  }

  auto &scanner = bwp::wallpaper::LibraryScanner::getInstance();
  scanner.setCallback(nullptr);

  // Only scan if we have paths
  if (!paths.empty()) {
    scanner.scan(paths);
  }

  g_timeout_add(
      2000,
      [](gpointer data) -> gboolean {
        auto *self = static_cast<LibraryView *>(data);
        auto &scanner = bwp::wallpaper::LibraryScanner::getInstance();

        self->refresh();

        if (scanner.isScanning()) {
          return G_SOURCE_CONTINUE;
        }
        return G_SOURCE_REMOVE;
      },
      this);
}

void LibraryView::refresh() {
  if (!m_grid)
    return;

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto wallpapers = lib.getAllWallpapers();
  for (const auto &info : wallpapers) {
    m_grid->addWallpaper(info);
  }
}

void LibraryView::onAddWallpaper() {
  // Get the parent window
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

  // Create file dialog (GTK4 style)
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Select Wallpaper");
  gtk_file_dialog_set_modal(dialog, TRUE);

  // Set up file filters
  GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);

  // All supported formats filter
  GtkFileFilter *allFilter = gtk_file_filter_new();
  gtk_file_filter_set_name(allFilter, "All Supported Formats");
  gtk_file_filter_add_mime_type(allFilter, "image/png");
  gtk_file_filter_add_mime_type(allFilter, "image/jpeg");
  gtk_file_filter_add_mime_type(allFilter, "image/webp");
  gtk_file_filter_add_mime_type(allFilter, "image/gif");
  gtk_file_filter_add_mime_type(allFilter, "image/bmp");
  gtk_file_filter_add_mime_type(allFilter, "video/mp4");
  gtk_file_filter_add_mime_type(allFilter, "video/webm");
  gtk_file_filter_add_mime_type(allFilter, "video/x-matroska");
  g_list_store_append(filters, allFilter);
  g_object_unref(allFilter);

  // Images only filter
  GtkFileFilter *imageFilter = gtk_file_filter_new();
  gtk_file_filter_set_name(imageFilter, "Images");
  gtk_file_filter_add_mime_type(imageFilter, "image/*");
  g_list_store_append(filters, imageFilter);
  g_object_unref(imageFilter);

  // Videos only filter
  GtkFileFilter *videoFilter = gtk_file_filter_new();
  gtk_file_filter_set_name(videoFilter, "Videos");
  gtk_file_filter_add_mime_type(videoFilter, "video/*");
  g_list_store_append(filters, videoFilter);
  g_object_unref(videoFilter);

  gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
  g_object_unref(filters);

  // Open dialog asynchronously
  gtk_file_dialog_open(
      dialog, window, nullptr,
      [](GObject *source, GAsyncResult *result, gpointer user_data) {
        auto *self = static_cast<LibraryView *>(user_data);
        GtkFileDialog *dlg = GTK_FILE_DIALOG(source);

        GError *error = nullptr;
        GFile *file = gtk_file_dialog_open_finish(dlg, result, &error);

        if (file) {
          char *path = g_file_get_path(file);
          if (path) {
            LOG_INFO("Adding wallpaper from file: " + std::string(path));

            // Add to library
            auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
            bwp::wallpaper::WallpaperInfo info;
            info.path = path;
            info.id = std::filesystem::path(path).stem().string();
            info.source = "local";

            // Detect type from extension
            std::string ext = bwp::utils::FileUtils::getExtension(path);
            if (ext == "mp4" || ext == "webm" || ext == "mkv") {
              info.type = bwp::wallpaper::WallpaperType::Video;
            } else if (ext == "gif") {
              info.type = bwp::wallpaper::WallpaperType::AnimatedImage;
            } else {
              info.type = bwp::wallpaper::WallpaperType::StaticImage;
            }

            lib.addWallpaper(info);

            // Refresh grid
            if (self->m_grid) {
              self->m_grid->addWallpaper(info);
            }

            g_free(path);
          }
          g_object_unref(file);
        } else if (error && error->code != GTK_DIALOG_ERROR_DISMISSED) {
          LOG_ERROR("File dialog error: " + std::string(error->message));
        }

        if (error) {
          g_error_free(error);
        }
      },
      this);

  g_object_unref(dialog);
}



void LibraryView::onSourceFilterChanged(const std::string &source) {
    if (m_grid) {
        m_grid->setFilterSource(source);
    }
}

} // namespace bwp::gui
