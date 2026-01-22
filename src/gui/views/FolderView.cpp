#include "FolderView.hpp"
#include "../../core/wallpaper/FolderManager.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <iostream>

namespace bwp::gui {

FolderView::FolderView() { setupUi(); }

FolderView::~FolderView() {}

void FolderView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_top(m_box, 12);
  gtk_widget_set_margin_start(m_box, 12);
  gtk_widget_set_margin_end(m_box, 12);

  // Header
  m_titleLabel = gtk_label_new("Folder");
  gtk_widget_add_css_class(m_titleLabel, "title-1");
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(m_box), m_titleLabel);

  m_grid = std::make_unique<WallpaperGrid>();
  gtk_box_append(GTK_BOX(m_box), m_grid->getWidget());
}

void FolderView::loadFolder(const std::string &folderId) {
  m_currentFolderId = folderId;
  refresh();
}

void FolderView::refresh() {
  m_grid->clear();

  auto &fm = bwp::wallpaper::FolderManager::getInstance();
  auto folderOpt = fm.getFolder(m_currentFolderId);

  if (folderOpt) {
    auto folder = *folderOpt;
    gtk_label_set_text(GTK_LABEL(m_titleLabel), folder.name.c_str());

    auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
    for (const auto &wpId : folder.wallpaperIds) {
      auto wp = lib.getWallpaper(wpId);
      if (wp) {
        m_grid->addWallpaper(*wp);
      }
    }
  } else {
    gtk_label_set_text(GTK_LABEL(m_titleLabel), "Folder not found");
  }
}

} // namespace bwp::gui
