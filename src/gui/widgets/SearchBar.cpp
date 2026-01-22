#include "SearchBar.hpp"

namespace bwp::gui {

SearchBar::SearchBar() {
  m_entry = gtk_search_entry_new();
  gtk_widget_set_hexpand(m_entry, TRUE);
  gtk_widget_set_margin_start(m_entry, 12);
  gtk_widget_set_margin_end(m_entry, 12);
  gtk_widget_set_margin_top(m_entry, 12);
  gtk_widget_set_margin_bottom(m_entry, 12);

  g_signal_connect(m_entry, "search-changed", G_CALLBACK(onChanged), this);
}

SearchBar::~SearchBar() {}

std::string SearchBar::getText() const {
  return gtk_editable_get_text(GTK_EDITABLE(m_entry));
}

void SearchBar::setText(const std::string &text) {
  gtk_editable_set_text(GTK_EDITABLE(m_entry), text.c_str());
}

void SearchBar::setCallback(SearchCallback callback) { m_callback = callback; }

void SearchBar::onChanged(GtkSearchEntry *entry, gpointer user_data) {
  auto *self = static_cast<SearchBar *>(user_data);
  if (self->m_callback) {
    self->m_callback(gtk_editable_get_text(GTK_EDITABLE(entry)));
  }
}

} // namespace bwp::gui
