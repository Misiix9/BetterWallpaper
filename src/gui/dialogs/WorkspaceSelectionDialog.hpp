#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <set>
#include <string>
namespace bwp::gui {
class WorkspaceSelectionDialog {
public:
  using Callback = std::function<void(const std::set<int> &selectedWorkspaces)>;
  WorkspaceSelectionDialog(GtkWindow *parent, const std::string &wallpaperPath);
  ~WorkspaceSelectionDialog();
  void show(Callback callback);
private:
  void setupUi();
  GtkWidget *m_dialog;
  std::string m_wallpaperPath;
  std::vector<GtkWidget *> m_checkboxes;
  Callback m_callback;
};
}  
