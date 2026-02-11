#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <map>
#include <string>
namespace bwp::gui {
class Sidebar {
public:
  Sidebar();
  ~Sidebar();
  GtkWidget *getWidget() const { return m_box; }
  using NavigationCallback = std::function<void(const std::string &page)>;
  void setCallback(NavigationCallback callback);
  void refresh();
  void updateBadge(const std::string &id, int count);
private:
  void setupUi();
  void addRow(const std::string &id, const std::string &iconName,
              const std::string &title);
  static void onRowActivated(GtkListBox *box, GtkListBoxRow *row,
                             gpointer user_data);
  GtkWidget *m_box;
  GtkWidget *m_listBox;
  NavigationCallback m_callback;
  std::map<GtkListBoxRow *, std::string> m_rowIds;
  std::map<std::string, GtkWidget *> m_badges;
};
}  
