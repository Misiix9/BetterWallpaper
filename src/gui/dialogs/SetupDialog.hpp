#pragma once

#include <adwaita.h>
#include <gtk/gtk.h>
#include <string>

namespace bwp::gui {

class SetupDialog {
public:
  SetupDialog(GtkWindow *parent);
  ~SetupDialog();

  void show();

private:
  GtkWidget *m_dialog;
  GtkWidget *m_stack;

  // Pages
  void createWelcomePage();
  void createMonitorsPage();
  void createLibraryPage();
  void createFinishPage();

  GtkWidget *m_libraryPathLabel;
  std::string m_selectedPath;
};

} // namespace bwp::gui
