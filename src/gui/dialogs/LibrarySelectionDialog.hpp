#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <string>
namespace bwp::gui {
class LibrarySelectionDialog {
public:
  using Callback = std::function<void(const std::string &selectedPath)>;
  LibrarySelectionDialog(GtkWindow *parent);
  ~LibrarySelectionDialog();
  void show(Callback callback);
private:
  void setupUi();
  GtkWidget *m_dialog;  
  GtkWidget *m_contentBox;
  Callback m_callback;
};
}  
