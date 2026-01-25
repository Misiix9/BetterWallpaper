#pragma once
#include "../widgets/WallpaperGrid.hpp"
#include <gtk/gtk.h>

namespace bwp::gui {

class RecentView {
public:
  RecentView();
  ~RecentView();

  GtkWidget *getWidget() const { return m_box; }

  void refresh();

private:
  void setupUi();
  void loadRecent();

  GtkWidget *m_box;
  std::unique_ptr<WallpaperGrid> m_grid;
};

} // namespace bwp::gui
