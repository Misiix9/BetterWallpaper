#pragma once
#include "../widgets/WallpaperGrid.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>

namespace bwp::gui {

class KeybindsView {
public:
  KeybindsView();
  ~KeybindsView() = default;

  GtkWidget *getWidget() const { return m_box; }

private:
  void setupUi();
  void populateList();
  void onCaptureKeybind(const std::string& action);

  GtkWidget *m_box;
  GtkWidget *m_listBox;
  
  // For capture dialog
  std::string m_pendingAction;
  GtkWidget *m_captureDialog;
};

} // namespace bwp::gui
