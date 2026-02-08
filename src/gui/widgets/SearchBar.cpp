#include "SearchBar.hpp"
#include "../../core/config/ConfigManager.hpp"
#include <algorithm>
#include <vector>

namespace bwp::gui {

SearchBar::SearchBar() {
  m_entry = gtk_search_entry_new();
  gtk_widget_set_hexpand(m_entry, TRUE);
  gtk_widget_set_margin_start(m_entry, 0);
  gtk_widget_set_margin_end(m_entry, 4);
  gtk_widget_set_margin_top(m_entry, 0);
  gtk_widget_set_margin_bottom(m_entry, 0);

  // Compact styling
  gtk_widget_add_css_class(m_entry, "search-bar");

  // History Popover
  m_popover = gtk_popover_new();
  gtk_widget_set_parent(m_popover, m_entry);
  gtk_popover_set_position(GTK_POPOVER(m_popover), GTK_POS_BOTTOM);
  gtk_popover_set_autohide(GTK_POPOVER(m_popover), TRUE);

  m_historyBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request(m_historyBox, 200, -1);
  gtk_popover_set_child(GTK_POPOVER(m_popover), m_historyBox);

  // Setup Entry Icon to Show History
  gtk_entry_set_icon_from_icon_name(
      GTK_ENTRY(m_entry), GTK_ENTRY_ICON_SECONDARY, "go-down-symbolic");
  gtk_entry_set_icon_tooltip_text(GTK_ENTRY(m_entry), GTK_ENTRY_ICON_SECONDARY,
                                  "Recent Searches");

  g_signal_connect(m_entry, "icon-press",
                   G_CALLBACK(+[](GtkEntry *, GtkEntryIconPosition pos,
                                  GdkEvent *, gpointer user_data) {
                     if (pos == GTK_ENTRY_ICON_SECONDARY) {
                       auto *self = static_cast<SearchBar *>(user_data);
                       self->updateHistoryMenu();
                       gtk_popover_popup(GTK_POPOVER(self->m_popover));
                     }
                   }),
                   this);

  g_signal_connect(m_entry, "search-changed", G_CALLBACK(onChanged), this);
  g_signal_connect(m_entry, "activate", G_CALLBACK(onEntryActivate), this);
}

SearchBar::~SearchBar() {
  if (m_timeoutId > 0) {
    g_source_remove(m_timeoutId);
  }
}

std::string SearchBar::getText() const {
  return gtk_editable_get_text(GTK_EDITABLE(m_entry));
}

void SearchBar::setText(const std::string &text) {
  gtk_editable_set_text(GTK_EDITABLE(m_entry), text.c_str());
}

void SearchBar::setCallback(SearchCallback callback) { m_callback = callback; }

void SearchBar::onChanged(GtkSearchEntry *, gpointer user_data) {
  auto *self = static_cast<SearchBar *>(user_data);

  if (self->m_timeoutId > 0) {
    g_source_remove(self->m_timeoutId);
    self->m_timeoutId = 0;
  }

  self->m_timeoutId = g_timeout_add(300, onTimeout, self);
}

gboolean SearchBar::onTimeout(gpointer user_data) {
  auto *self = static_cast<SearchBar *>(user_data);
  if (self->m_callback) {
    self->m_callback(self->getText());
  }
  self->m_timeoutId = 0;
  return FALSE;
}

void SearchBar::onEntryActivate(GtkSearchEntry * /*entry*/,
                                gpointer user_data) {
  auto *self = static_cast<SearchBar *>(user_data);
  std::string text = self->getText();
  if (!text.empty()) {
    self->addToHistory(text);
    // Force callback immediately on enter?
    if (self->m_callback)
      self->m_callback(text);
  }
}

void SearchBar::addToHistory(const std::string &query) {
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto history = conf.get<std::vector<std::string>>("search.history");

  // Remove if exists (to move to top)
  auto it = std::remove(history.begin(), history.end(), query);
  history.erase(it, history.end());

  // Add to front
  history.insert(history.begin(), query);

  // Limit to 10
  if (history.size() > 10)
    history.resize(10);

  conf.set("search.history", history);
}

void SearchBar::updateHistoryMenu() {
  // Clear
  GtkWidget *child = gtk_widget_get_first_child(m_historyBox);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(m_historyBox), child);
    child = next;
  }

  auto &conf = bwp::config::ConfigManager::getInstance();
  auto history = conf.get<std::vector<std::string>>("search.history");

  if (history.empty()) {
    GtkWidget *lbl = gtk_label_new("No recent searches");
    gtk_widget_set_margin_top(lbl, 8);
    gtk_widget_set_margin_bottom(lbl, 8);
    gtk_widget_add_css_class(lbl, "dim-label");
    gtk_box_append(GTK_BOX(m_historyBox), lbl);
    return;
  }

  for (const auto &item : history) {
    GtkWidget *btn = gtk_button_new_with_label(item.c_str());
    gtk_widget_add_css_class(btn, "flat");
    gtk_widget_set_halign(btn, GTK_ALIGN_FILL);

    // Ccap the label inside if needed, simpler to just use button label
    g_object_set_data_full(G_OBJECT(btn), "history_text",
                           g_strdup(item.c_str()), g_free);

    g_signal_connect(
        btn, "clicked", G_CALLBACK(+[](GtkButton *b, gpointer data) {
          SearchBar *self = static_cast<SearchBar *>(data);
          const char *txt =
              (const char *)g_object_get_data(G_OBJECT(b), "history_text");
          if (txt) {
            self->setText(txt); // will trigger search-changed -> debounce
            gtk_popover_popdown(GTK_POPOVER(self->m_popover));
          }
        }),
        this);

    gtk_box_append(GTK_BOX(m_historyBox), btn);
  }

  // Clear button
  if (!history.empty()) {
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(m_historyBox), sep);

    GtkWidget *clearBtn = gtk_button_new_with_label("Clear History");
    gtk_widget_add_css_class(clearBtn, "flat");
    gtk_widget_add_css_class(clearBtn, "destructive-action");
    g_signal_connect(clearBtn, "clicked",
                     G_CALLBACK(+[](GtkButton *, gpointer data) {
                       bwp::config::ConfigManager::getInstance().set(
                           "search.history", std::vector<std::string>());
                       static_cast<SearchBar *>(data)->updateHistoryMenu();
                     }),
                     this);
    gtk_box_append(GTK_BOX(m_historyBox), clearBtn);
  }
}

} // namespace bwp::gui
