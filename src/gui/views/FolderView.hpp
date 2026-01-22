#pragma once
#include "../widgets/WallpaperGrid.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>

namespace bwp::gui {

class FolderView {
public:
  FolderView();
  ~FolderView();

  GtkWidget *getWidget() const { return m_box; }

  void loadFolder(const std::string &folderId); // Load specific folder
  void refresh();

private:
  void setupUi();

  GtkWidget *m_box;
  GtkWidget *m_titleLabel;
  std::unique_ptr<WallpaperGrid> m_grid;
  std::string m_currentFolderId;
};

} // namespace bwp::gui
