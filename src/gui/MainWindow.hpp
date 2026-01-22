#pragma once
#include "views/FolderView.hpp"
#include "views/LibraryView.hpp"
#include "views/MonitorsView.hpp"
#include "views/WorkshopView.hpp"
#include "widgets/Sidebar.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>

namespace bwp::gui {

class MainWindow {
public:
  MainWindow(AdwApplication *app);
  ~MainWindow();

  void show();

private:
  void setupUi();

  GtkWidget *m_window;
  GtkWidget *m_splitView; // AdwOverlaySplitView or similar
  GtkWidget *m_contentStack;

  std::unique_ptr<Sidebar> m_sidebar;
  std::unique_ptr<LibraryView> m_libraryView;
  std::unique_ptr<MonitorsView> m_monitorsView;
  std::unique_ptr<MonitorsView> m_monitorsView;
  std::unique_ptr<FolderView> m_folderView;
  std::unique_ptr<WorkshopView> m_workshopView;
};

} // namespace bwp::gui
