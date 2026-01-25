#include "Sidebar.hpp"
#include "../../core/wallpaper/FolderManager.hpp"
#include <adwaita.h>
#include <iostream>

namespace bwp::gui {

Sidebar::Sidebar() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_box, 200, -1);

  // Title/Header (optional if split view handles it)
  // Sidebar list
  m_listBox = gtk_list_box_new();
  gtk_widget_add_css_class(m_listBox, "navigation-sidebar");

  // Add items
  addRow("library", "folder-pictures-symbolic", "Library");
  addRow("favorites", "starred-symbolic", "Favorites");
  addRow("recent", "document-open-recent-symbolic", "Recent");
  addRow("workshop", "applications-internet-symbolic", "Workshop");

  gtk_box_append(GTK_BOX(m_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  // Folders header
  GtkWidget *folderLabel = gtk_label_new("Folders");
  gtk_widget_add_css_class(folderLabel, "heading");
  gtk_widget_set_halign(folderLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_start(folderLabel, 12);
  gtk_widget_set_margin_top(folderLabel, 8);
  gtk_box_append(GTK_BOX(m_box), folderLabel);

  // Dynamic Folders
  auto folders = bwp::wallpaper::FolderManager::getInstance().getFolders();
  for (const auto &folder : folders) {
    addRow("folder." + folder.id, folder.icon, folder.name);
  }

  // Add new folder button (mockup for now action-wise)
  addRow("folder.new", "list-add-symbolic", "New Folder...");

  gtk_box_append(GTK_BOX(m_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  // Placeholder for folders
  // addRow(...)

  gtk_box_append(GTK_BOX(m_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  addRow("monitors", "video-display-symbolic", "Monitors");
  addRow("profiles", "document-properties-symbolic", "Profiles");
  addRow("schedule", "alarm-symbolic", "Schedule");
  addRow("settings", "emblem-system-symbolic", "Settings");

  gtk_box_append(GTK_BOX(m_box), m_listBox);

  g_signal_connect(m_listBox, "row-activated", G_CALLBACK(onRowActivated),
                   this);

  // Select first
  GtkListBoxRow *first =
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), 0);
  if (first)
    gtk_list_box_select_row(GTK_LIST_BOX(m_listBox), first);
}

Sidebar::~Sidebar() {
  // Widget owned by GTK parent usually
}

void Sidebar::addRow(const std::string &id, const std::string &iconName,
                     const std::string &title) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);

  GtkWidget *icon = gtk_image_new_from_icon_name(iconName.c_str());
  GtkWidget *label = gtk_label_new(title.c_str());

  gtk_box_append(GTK_BOX(box), icon);
  gtk_box_append(GTK_BOX(box), label);

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
  gtk_list_box_append(GTK_LIST_BOX(m_listBox), row);

  m_rowIds[GTK_LIST_BOX_ROW(row)] = id;
}

void Sidebar::onRowActivated(GtkListBox *, GtkListBoxRow *row,
                             gpointer user_data) {
  auto *self = static_cast<Sidebar *>(user_data);
  if (!row)
    return;

  if (self->m_rowIds.count(row)) {
    std::string id = self->m_rowIds[row];
    if (self->m_callback) {
      self->m_callback(id);
    }
  }
}

void Sidebar::setCallback(NavigationCallback callback) {
  m_callback = callback;
}

} // namespace bwp::gui
