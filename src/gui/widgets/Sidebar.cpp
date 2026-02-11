#include "Sidebar.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/slideshow/SlideshowManager.hpp"
#include "../../core/wallpaper/FolderManager.hpp"
#include <adwaita.h>
#include <iostream>
#include <cstdlib>
#include <string>

namespace bwp::gui {

Sidebar::Sidebar() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_box, 220, -1);
  gtk_widget_set_vexpand(m_box, TRUE);
  gtk_widget_add_css_class(m_box, "sidebar-container");

  m_listBox = gtk_list_box_new();
  gtk_widget_add_css_class(m_listBox, "sidebar");
  gtk_widget_set_vexpand(m_listBox, TRUE);

  g_signal_connect(m_listBox, "row-activated", G_CALLBACK(onRowActivated),
                   this);

  refresh();

  gtk_box_append(GTK_BOX(m_box), m_listBox);

  // Select first
  GtkListBoxRow *first =
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), 0);
  if (first)
    gtk_list_box_select_row(GTK_LIST_BOX(m_listBox), first);
}

void Sidebar::refresh() {
  // Block signals to prevent callbacks during destruction
  g_signal_handlers_block_by_func(m_listBox, (gpointer)onRowActivated, this);

  // Clear existing items
  GtkWidget *child = gtk_widget_get_first_child(m_listBox);
  while (child != nullptr) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(m_listBox), child);
    child = next;
  }
  m_rowIds.clear();

  // Add items (Favorites/Workshop removed - use filter bar instead)
  addRow("library", "folder-pictures-symbolic", "Library");

  // Separator logic handled by CSS or just spacing in listbox
  // For proper separators inside listbox, we can add them as non-selectable
  // rows or just use spacing

  // Folders header
  GtkWidget *folderHeaderRow = gtk_list_box_row_new();
  gtk_widget_set_sensitive(folderHeaderRow, FALSE);
  gtk_widget_set_focusable(folderHeaderRow, FALSE);

  GtkWidget *folderLabel = gtk_label_new("Folders");
  gtk_widget_add_css_class(folderLabel, "heading");
  gtk_widget_set_halign(folderLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_start(folderLabel, 12);
  gtk_widget_set_margin_top(folderLabel, 16);
  gtk_widget_set_margin_bottom(folderLabel, 4);

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(folderHeaderRow), folderLabel);
  gtk_list_box_append(GTK_LIST_BOX(m_listBox), folderHeaderRow);

  // Dynamic Folders
  auto folders = bwp::wallpaper::FolderManager::getInstance().getFolders();
  for (const auto &folder : folders) {
    addRow("folder." + folder.id, folder.icon, folder.name);
  }

  // Add new folder button
  addRow("folder.new", "list-add-symbolic", "New Folder...");

  // Monitors header
  GtkWidget *monHeaderRow = gtk_list_box_row_new();
  gtk_widget_set_sensitive(monHeaderRow, FALSE);
  gtk_widget_set_focusable(monHeaderRow, FALSE);

  GtkWidget *monLabel = gtk_label_new("System");
  gtk_widget_add_css_class(monLabel, "heading");
  gtk_widget_set_halign(monLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_start(monLabel, 12);
  gtk_widget_set_margin_top(monLabel, 16);
  gtk_widget_set_margin_bottom(monLabel, 4);

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(monHeaderRow), monLabel);
  gtk_list_box_append(GTK_LIST_BOX(m_listBox), monHeaderRow);

  addRow("monitors", "video-display-symbolic", "Monitors");

  // Only show Hyprland settings if we are in a Hyprland session
  const char *currentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
  std::string session = currentDesktop ? currentDesktop : "";
  if (session.find("Hyprland") != std::string::npos) {
    addRow("hyprland", "preferences-desktop-remote-desktop-symbolic",
           "Hyprland");
  }

  addRow("profiles", "document-properties-symbolic", "Profiles");
  addRow("schedule", "alarm-symbolic", "Schedule");
  addRow("settings", "emblem-system-symbolic", "Settings");

  // Unblock signals
  g_signal_handlers_unblock_by_func(m_listBox, (gpointer)onRowActivated, this);
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
  gtk_widget_set_hexpand(label, TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_START);

  // Badge Label
  GtkWidget *badge = gtk_label_new("");
  gtk_widget_add_css_class(badge, "badge"); // Ensure CSS has .badge
  gtk_widget_set_visible(badge, FALSE);
  m_badges[id] = badge;

  gtk_box_append(GTK_BOX(box), icon);
  gtk_box_append(GTK_BOX(box), label);
  gtk_box_append(GTK_BOX(box), badge);

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
  gtk_list_box_append(GTK_LIST_BOX(m_listBox), row);

  m_rowIds[GTK_LIST_BOX_ROW(row)] = id;

  // Context Menu
  if (id.rfind("folder.", 0) == 0 && id != "folder.new") {
    // ... (existing context menu logic)
    GtkGesture *rightClick = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(rightClick), 3);

    // Pass ID copy
    std::string *idPtr = new std::string(id);
    g_object_set_data_full(G_OBJECT(row), "folder_id", idPtr, [](gpointer d) {
      delete static_cast<std::string *>(d);
    });

    auto rightClickHandler =
        +[](GtkGestureClick *gesture, int, double x, double y, gpointer) {
          GtkWidget *widget =
              gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
          std::string *fid = static_cast<std::string *>(
              g_object_get_data(G_OBJECT(widget), "folder_id"));

          if (fid) {
            GtkWidget *popover = gtk_popover_new();
            gtk_widget_set_parent(popover, widget);

            GtkWidget *menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_popover_set_child(GTK_POPOVER(popover), menuBox);

            // "Play Slideshow" button
            GtkWidget *playBtn = gtk_button_new();
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(hbox), gtk_image_new_from_icon_name(
                                              "media-playback-start-symbolic"));
            gtk_box_append(GTK_BOX(hbox), gtk_label_new("Play Slideshow"));
            gtk_button_set_child(GTK_BUTTON(playBtn), hbox);
            gtk_widget_add_css_class(playBtn, "flat");

            // Button data
            std::string folderId = fid->substr(7); // remove "folder."
            std::string *fIdArg = new std::string(folderId);
            g_object_set_data_full(
                G_OBJECT(playBtn), "fid", fIdArg,
                [](gpointer d) { delete static_cast<std::string *>(d); });

            g_signal_connect(
                playBtn, "clicked", G_CALLBACK(+[](GtkButton *b, gpointer) {
                  std::string *fid = static_cast<std::string *>(
                      g_object_get_data(G_OBJECT(b), "fid"));
                  if (fid) {
                    auto folderMsg =
                        bwp::wallpaper::FolderManager::getInstance().getFolder(
                            *fid);
                    if (folderMsg && !folderMsg->wallpaperIds.empty()) {
                      auto &conf = bwp::config::ConfigManager::getInstance();
                      int interval = conf.get<int>("slideshow.interval");
                      if (interval <= 0)
                        interval = 300;

                      auto &sm = bwp::core::SlideshowManager::getInstance();
                      sm.setShuffle(conf.get<bool>("slideshow.shuffle"));
                      sm.start(folderMsg->wallpaperIds, interval);
                    }
                  }
                  GtkWidget *pop =
                      gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
                  if (pop)
                    gtk_popover_popdown(GTK_POPOVER(pop));
                }),
                nullptr);

            gtk_box_append(GTK_BOX(menuBox), playBtn);

            // Positioning
            GdkRectangle rect = {(int)x, (int)y, 1, 1};
            gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
            gtk_popover_popup(GTK_POPOVER(popover));
          }
        };

    g_signal_connect(rightClick, "pressed", G_CALLBACK(rightClickHandler),
                     nullptr);

    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(rightClick));
  }
}

void Sidebar::updateBadge(const std::string &id, int count) {
  if (m_badges.count(id)) {
    GtkWidget *badge = m_badges[id];
    if (count > 0) {
      gtk_label_set_text(GTK_LABEL(badge), std::to_string(count).c_str());
      gtk_widget_set_visible(badge, TRUE);
    } else {
      gtk_widget_set_visible(badge, FALSE);
    }
  }
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
