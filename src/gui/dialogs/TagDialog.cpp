#include "TagDialog.hpp"
#include "../../core/wallpaper/TagManager.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <algorithm>

namespace bwp::gui {

TagDialog::TagDialog(GtkWindow *parent, const std::string &wallpaperId)
    : m_wallpaperId(wallpaperId) {
  // Use AdwWindow for modal dialog in 1.4- style or simple GtkWindow
  m_dialog = adw_window_new();
  gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
  gtk_window_set_default_size(GTK_WINDOW(m_dialog), 400, 500);
  gtk_window_set_title(GTK_WINDOW(m_dialog), "Manage Tags");

  setupUi();
  loadTags();
}

TagDialog::~TagDialog() {
  // Widgets destroyed by window
}

void TagDialog::show() { gtk_window_present(GTK_WINDOW(m_dialog)); }

void TagDialog::setupUi() {
  GtkWidget *content = adw_window_get_content(ADW_WINDOW(m_dialog));
  if (!content) {
    GtkWidget *page = adw_toolbar_view_new();
    adw_window_set_content(ADW_WINDOW(m_dialog), page);

    // Header
    GtkWidget *header = adw_header_bar_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(page), header);

    // Content box
    m_contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(m_contentBox, 24);
    gtk_widget_set_margin_end(m_contentBox, 24);
    gtk_widget_set_margin_top(m_contentBox, 24);
    gtk_widget_set_margin_bottom(m_contentBox, 24);

    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(page), m_contentBox);
  }

  // Entry
  m_tagEntry = adw_entry_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_tagEntry), "Add Tag");
  adw_entry_row_set_show_apply_button(ADW_ENTRY_ROW(m_tagEntry), TRUE);

  g_signal_connect(m_tagEntry, "apply",
                   G_CALLBACK(+[](AdwEntryRow *row, gpointer user_data) {
                     auto *self = static_cast<TagDialog *>(user_data);
                     const char *text =
                         gtk_editable_get_text(GTK_EDITABLE(row));
                     if (text && *text) {
                       self->addTag(text);
                       gtk_editable_set_text(GTK_EDITABLE(row), "");
                     }
                   }),
                   this);

  GtkWidget *group = adw_preferences_group_new();
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), m_tagEntry);
  gtk_box_append(GTK_BOX(m_contentBox), group);

  // Flowbox for tags
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_box_append(GTK_BOX(m_contentBox), scroll);

  m_tagsFlowBox = gtk_flow_box_new();
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_tagsFlowBox),
                                  GTK_SELECTION_NONE);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_tagsFlowBox), 3);
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_tagsFlowBox), 1);
  gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(m_tagsFlowBox), 6);
  gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(m_tagsFlowBox), 6);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), m_tagsFlowBox);
}

void TagDialog::loadTags() {
  // Clear
  GtkWidget *child = gtk_widget_get_first_child(m_tagsFlowBox);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(m_tagsFlowBox), child);
    child = next;
  }

  // Get from library
  auto info = bwp::wallpaper::WallpaperLibrary::getInstance().getWallpaper(
      m_wallpaperId);
  if (!info)
    return;

  m_currentTags = info->tags;

  for (const auto &tag : m_currentTags) {
    GtkWidget *chip = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(chip), tag.c_str());

    GtkWidget *delBtn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(delBtn, "flat");

    // Signal with data copy? Capturing lambda with user_data trick or
    // g_signal_connect_data
    g_signal_connect(
        delBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer user_data) {
          auto *pair =
              static_cast<std::pair<TagDialog *, std::string> *>(user_data);
          pair->first->removeTag(pair->second);
          delete pair; // Single use? No, this is dangerous if clicked multiple
                       // times (impossible as detached).
                       // Better to store pointer and refresh.
        }),
        new std::pair<TagDialog *, std::string>(this, tag));

    adw_action_row_add_suffix(ADW_ACTION_ROW(chip), delBtn);
    gtk_flow_box_append(GTK_FLOW_BOX(m_tagsFlowBox), chip);
  }
}

void TagDialog::addTag(const std::string &tag) {
  bwp::wallpaper::TagManager::getInstance().tagWallpaper(m_wallpaperId, tag);
  loadTags();
}

void TagDialog::removeTag(const std::string &tag) {
  bwp::wallpaper::TagManager::getInstance().untagWallpaper(m_wallpaperId, tag);
  loadTags();
}

} // namespace bwp::gui
