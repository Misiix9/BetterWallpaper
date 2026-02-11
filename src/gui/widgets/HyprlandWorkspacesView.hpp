#pragma once
#include <adwaita.h>
#include <gtk/gtk.h>
#include <map>
#include <string>
namespace bwp::gui {
class HyprlandWorkspacesView {
public:
  HyprlandWorkspacesView();
  ~HyprlandWorkspacesView();
  GtkWidget *getWidget() const { return m_box; }
  void refresh();
private:
  void setupUi();
  void addWorkspaceRow(int workspaceId, const std::string &wallpaperPath);
  void onSelectWallpaper(int workspaceId, GtkWidget *row);
  void showConfigSnippet();
  GtkWidget *m_box;
  GtkWidget *m_workspaceList;
  GtkWidget *m_banner;
  std::map<int, GtkWidget *> m_workspaceRows;
};
}  
