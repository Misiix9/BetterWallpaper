#include "FolderView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/slideshow/SlideshowManager.hpp"
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
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append(GTK_BOX(m_box), headerBox);
  m_titleLabel = gtk_label_new("Folder");
  gtk_widget_add_css_class(m_titleLabel, "title-1");
  gtk_widget_set_halign(m_titleLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(m_titleLabel, TRUE);
  gtk_box_append(GTK_BOX(headerBox), m_titleLabel);
  GtkWidget *playBtn =
      gtk_button_new_from_icon_name("media-playback-start-symbolic");
  gtk_widget_set_tooltip_text(playBtn, "Start Folder Slideshow");
  gtk_widget_add_css_class(playBtn, "flat");
  gtk_box_append(GTK_BOX(headerBox), playBtn);
  g_signal_connect(playBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     FolderView *self = static_cast<FolderView *>(data);
                     auto &fm = bwp::wallpaper::FolderManager::getInstance();
                     auto folderOpt = fm.getFolder(self->m_currentFolderId);
                     if (folderOpt && !folderOpt->wallpaperIds.empty()) {
                       auto &conf = bwp::config::ConfigManager::getInstance();
                       int interval = conf.get<int>("slideshow.interval");
                       if (interval <= 0)
                         interval = 300;  
                       auto &sm = bwp::core::SlideshowManager::getInstance();
                       sm.setShuffle(conf.get<bool>("slideshow.shuffle"));
                       sm.start(folderOpt->wallpaperIds, interval);
                     }
                   }),
                   this);
  m_grid = std::make_unique<WallpaperGrid>();
  m_grid->setReorderCallback(
      [this](const std::string &src, const std::string &tgt, bool after) {
        if (!m_currentFolderId.empty()) {
          bwp::wallpaper::FolderManager::getInstance().reorderWallpaper(
              m_currentFolderId, src, tgt, after);
          refresh();
        }
      });
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
}  
