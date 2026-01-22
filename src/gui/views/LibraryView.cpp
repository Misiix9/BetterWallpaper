#include "LibraryView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/FileUtils.hpp"
#include "../../core/utils/Logger.hpp"
#include <filesystem>

namespace bwp::gui {

LibraryView::LibraryView() {
  setupUi();
  loadWallpapers();
}

LibraryView::~LibraryView() {}

void LibraryView::setupUi() {
  // Main layout
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Header/Search area
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_margin_bottom(headerBox, 0);

  m_searchBar = std::make_unique<SearchBar>();
  m_searchBar->setCallback([this](const std::string &text) {
    if (m_grid)
      m_grid->filter(text);
  });

  gtk_box_append(GTK_BOX(headerBox), m_searchBar->getWidget());
  gtk_box_append(GTK_BOX(m_box), headerBox);

  // Grid
  m_grid = std::make_unique<WallpaperGrid>();
  gtk_box_append(GTK_BOX(m_box), m_grid->getWidget());
}

void LibraryView::loadWallpapers() {
  // Initialize library (load from disk)
  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  lib.initialize();

  // Start scanner for configured paths
  auto paths =
      bwp::config::ConfigManager::getInstance().get<std::vector<std::string>>(
          "library.paths");
  if (paths.empty()) {
    paths.push_back("~/Pictures");
  }

  auto &scanner = bwp::wallpaper::LibraryScanner::getInstance();
  // Connect callback to refreshing grid when new file is found
  scanner.setCallback([this](const std::string &path) {
    // This runs in scanner thread, need to dispatch to main thread if updating
    // UI directly Or simply let the refresh logic handle it if triggered.
    // Ideally we use Glib::Dispatcher or g_idle_add.
    g_idle_add(
        +[](gpointer data) -> gboolean {
          auto *self = static_cast<LibraryView *>(data);
          self->refresh();
          return G_SOURCE_REMOVE;
        },
        this);
  });
  scanner.scan(paths);

  // Initial load from DB
  auto wallpapers = lib.getAllWallpapers();
  for (const auto &info : wallpapers) {
    m_grid->addWallpaper(info);
  }
}

void LibraryView::refresh() { loadWallpapers(); }

} // namespace bwp::gui
