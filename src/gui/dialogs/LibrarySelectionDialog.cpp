#include "LibrarySelectionDialog.hpp"
#include "../widgets/WallpaperGrid.hpp"
#include <adwaita.h>
namespace bwp::gui {
LibrarySelectionDialog::LibrarySelectionDialog(GtkWindow *parent) {
  m_dialog = adw_window_new();
  gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
  gtk_window_set_default_size(GTK_WINDOW(m_dialog), 900, 600);
  gtk_window_set_title(GTK_WINDOW(m_dialog), "Select Wallpaper");
  setupUi();
}
LibrarySelectionDialog::~LibrarySelectionDialog() {}
void LibrarySelectionDialog::setupUi() {
  GtkWidget *toolbar_view = adw_toolbar_view_new();
  adw_window_set_content(ADW_WINDOW(m_dialog), toolbar_view);
  GtkWidget *header = adw_header_bar_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header);
  GtkWidget *cancelBtn = gtk_button_new_with_label("Cancel");
  g_object_set_data(G_OBJECT(cancelBtn), "dialog", this);
  g_signal_connect(cancelBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<LibrarySelectionDialog *>(data);
                     gtk_window_close(GTK_WINDOW(self->m_dialog));
                   }),
                   this);
  adw_header_bar_pack_start(ADW_HEADER_BAR(header), cancelBtn);
  m_contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view), m_contentBox);
  auto *grid = new WallpaperGrid();
  GtkWidget *gridWidget = grid->getWidget();
  gtk_widget_set_vexpand(gridWidget, TRUE);
  gtk_box_append(GTK_BOX(m_contentBox), gridWidget);
  g_object_set_data_full(
      G_OBJECT(m_dialog), "grid_obj", grid,
      [](gpointer data) { delete static_cast<WallpaperGrid *>(data); });
  grid->setSelectionCallback([this](const bwp::wallpaper::WallpaperInfo &info) {
    if (m_callback) {
      m_callback(info.path);
    }
    gtk_window_close(GTK_WINDOW(m_dialog));
  });
}
void LibrarySelectionDialog::show(Callback callback) {
  m_callback = callback;
  gtk_window_present(GTK_WINDOW(m_dialog));
}
}  
