#include "LibraryView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/FileUtils.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <filesystem>

namespace bwp::gui {

// Fixed width for the preview panel
static constexpr int PREVIEW_PANEL_WIDTH = 320;

LibraryView::LibraryView() {
  setupUi();

  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<LibraryView *>(data);
        self->loadWallpapers();
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
  gtk_paned_set_end_child(GTK_PANED(paned), previewWidget);
  gtk_paned_set_resize_end_child(GTK_PANED(paned),
                                 FALSE); // Don't resize preview
  gtk_paned_set_shrink_end_child(GTK_PANED(paned),
                                 FALSE); // Don't shrink preview

  // Set initial position to give grid most of the space
  gtk_paned_set_position(GTK_PANED(paned), 700);

  // Connect Grid Selection
  m_grid->setSelectionCallback(
      [this](const bwp::wallpaper::WallpaperInfo &info) {
        if (m_previewPanel) {
          m_previewPanel->setWallpaper(info);
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

  auto paths =
      bwp::config::ConfigManager::getInstance().get<std::vector<std::string>>(
          "library.paths");
  if (paths.empty()) {
    paths.push_back("~/Pictures");
  }

  auto &scanner = bwp::wallpaper::LibraryScanner::getInstance();
  scanner.setCallback(nullptr);
  scanner.scan(paths);

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

} // namespace bwp::gui
