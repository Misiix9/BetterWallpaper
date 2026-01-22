#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>

namespace bwp::gui {

class TagDialog {
public:
  TagDialog(GtkWindow *parent, const std::string &wallpaperId);
  ~TagDialog();

  void show();

private:
  void setupUi();
  void loadTags();
  void addTag(const std::string &tag);
  void removeTag(const std::string &tag);

  GtkWidget *m_dialog; // AdwWindow or GtkWindow? AdwDialog if libadwaita 1.5+,
                       // else AdwWindow as modal.
  GtkWidget *m_contentBox;
  GtkWidget *m_tagEntry;
  GtkWidget *m_tagsFlowBox;

  std::string m_wallpaperId;
  std::vector<std::string> m_currentTags;
};

} // namespace bwp::gui
