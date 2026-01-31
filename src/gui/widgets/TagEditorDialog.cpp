#include "TagEditorDialog.hpp"
#include <algorithm>
#include <iostream>

namespace bwp::gui {

TagEditorDialog::TagEditorDialog(GtkWindow *parent) {
  m_dialog = adw_window_new();
  gtk_window_set_title(GTK_WINDOW(m_dialog), "Edit Tags");
  gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
  gtk_window_set_default_size(GTK_WINDOW(m_dialog), 400, 500);

  setupUi();
}

TagEditorDialog::~TagEditorDialog() {
  // Dialog is destroyed by GTK
}

void TagEditorDialog::setupUi() {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  adw_window_set_content(ADW_WINDOW(m_dialog), box);

  // Toolbar
  GtkWidget *header = adw_header_bar_new();
  // In AdwWindow, headerbar is top child usually, but here we construct content
  // manually Actually best to use AdwToolbarView if AdwWindow. Simplifying:
  // just put a box.

  // Title
  GtkWidget *titleLabel = gtk_label_new("Manage Tags");
  gtk_widget_add_css_class(titleLabel, "title-4");
  gtk_box_append(GTK_BOX(box), titleLabel);

  // Entry Row
  GtkWidget *inputGroup = adw_preferences_group_new();
  GtkWidget *entryRow = adw_entry_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entryRow), "Add Tag");
  m_entry = entryRow;

  // Add button
  GtkWidget *addBtn = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_set_valign(addBtn, GTK_ALIGN_CENTER);
  g_signal_connect(
      addBtn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<TagEditorDialog *>(data);
        const char *text = gtk_editable_get_text(GTK_EDITABLE(self->m_entry));
        if (text && text[0] != '\0') {
          self->addTag(text);
          gtk_editable_set_text(GTK_EDITABLE(self->m_entry), "");
        }
      }),
      this);
  adw_entry_row_add_suffix(ADW_ENTRY_ROW(entryRow), addBtn);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(inputGroup), entryRow);
  gtk_box_append(GTK_BOX(box), inputGroup);

  // Tags List Area
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_box_append(GTK_BOX(box), scroll);

  m_tagBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // Listbox style
  GtkWidget *listBox = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(listBox), GTK_SELECTION_NONE);
  gtk_widget_add_css_class(listBox, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listBox);
  m_tagBox = listBox;

  // Apply Button
  GtkWidget *applyBtn = gtk_button_new_with_label("Done");
  gtk_widget_add_css_class(applyBtn, "suggested-action");
  gtk_widget_set_size_request(applyBtn, -1, 40);
  g_signal_connect(applyBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<TagEditorDialog *>(data);
                     if (self->m_callback) {
                       self->m_callback(self->m_currentId, self->m_currentTags);
                     }
                     gtk_window_close(GTK_WINDOW(self->m_dialog));
                   }),
                   this);
  gtk_box_append(GTK_BOX(box), applyBtn);
}

void TagEditorDialog::show(const std::string &wallpaperId,
                           const std::vector<std::string> &currentTags) {
  m_currentId = wallpaperId;
  m_currentTags = currentTags;
  updateTagList();
  gtk_window_present(GTK_WINDOW(m_dialog));
}

void TagEditorDialog::setCallback(ApplyCallback callback) {
  m_callback = callback;
}

void TagEditorDialog::addTag(const std::string &tag) {
  // Check duplicate
  for (const auto &t : m_currentTags) {
    if (t == tag)
      return;
  }
  m_currentTags.push_back(tag);
  updateTagList();
}

void TagEditorDialog::removeTag(const std::string &tag) {
  auto it = std::remove(m_currentTags.begin(), m_currentTags.end(), tag);
  if (it != m_currentTags.end()) {
    m_currentTags.erase(it, m_currentTags.end());
    updateTagList();
  }
}

void TagEditorDialog::updateTagList() {
  // Clear list
  GtkWidget *child = gtk_widget_get_first_child(m_tagBox);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(m_tagBox), child);
    child = next;
  }

  // Populate
  for (const auto &tag : m_currentTags) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(rowBox, 12);
    gtk_widget_set_margin_end(rowBox, 12);
    gtk_widget_set_margin_top(rowBox, 12);
    gtk_widget_set_margin_bottom(rowBox, 12);

    GtkWidget *label = gtk_label_new(tag.c_str());
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(rowBox), label);

    GtkWidget *delBtn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(delBtn, "flat");
    gtk_widget_add_css_class(delBtn, "destructive-action");

    // Pass string copy to callback
    std::string *tagPtr = new std::string(tag);
    g_object_set_data_full(
        G_OBJECT(delBtn), "tag_str", tagPtr,
        [](gpointer data) { delete static_cast<std::string *>(data); });

    g_signal_connect(delBtn, "clicked",
                     G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *self = static_cast<TagEditorDialog *>(data);
                       std::string *t = static_cast<std::string *>(
                           g_object_get_data(G_OBJECT(btn), "tag_str"));
                       if (t)
                         self->removeTag(*t);
                     }),
                     this);

    gtk_box_append(GTK_BOX(rowBox), delBtn);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rowBox);
    gtk_list_box_append(GTK_LIST_BOX(m_tagBox), row);
  }
}

} // namespace bwp::gui
