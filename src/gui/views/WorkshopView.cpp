#include "WorkshopView.hpp"
#include "../../core/utils/Logger.hpp"
#include <iostream>

namespace bwp::gui {

WorkshopView::WorkshopView() { setupUi(); }

WorkshopView::~WorkshopView() {}

void WorkshopView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(m_box, 20);
  gtk_widget_set_margin_end(m_box, 20);
  gtk_widget_set_margin_top(m_box, 20);
  gtk_widget_set_margin_bottom(m_box, 20);

  // Header
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(m_box), headerBox);

  GtkWidget *title = gtk_label_new("Steam Workshop");
  gtk_widget_add_css_class(title, "title-2");
  gtk_box_append(GTK_BOX(headerBox), title);

  // Search Bar
  m_searchBar = gtk_search_entry_new();
  gtk_widget_set_hexpand(m_searchBar, TRUE);
  g_signal_connect(m_searchBar, "activate", G_CALLBACK(onSearchEnter), this);
  gtk_box_append(GTK_BOX(headerBox), m_searchBar);

  // Progress
  m_progressBar = gtk_progress_bar_new();
  gtk_box_append(GTK_BOX(m_box), m_progressBar);

  // Scrolled Grid
  m_scrolledWindow = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(m_scrolledWindow, TRUE);
  gtk_box_append(GTK_BOX(m_box), m_scrolledWindow);

  m_grid = gtk_flow_box_new();
  gtk_flow_box_set_valign(GTK_FLOW_BOX(m_grid), GTK_ALIGN_START);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_grid), 30); // Dynamic
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_grid), 1);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_grid), GTK_SELECTION_NONE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrolledWindow), m_grid);

  // Initial search
  performSearch("anime"); // Default query
}

void WorkshopView::onSearchEnter(GtkEntry *entry, gpointer user_data) {
  auto *self = static_cast<WorkshopView *>(user_data);
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
  self->performSearch(text);
}

void WorkshopView::performSearch(const std::string &query) {
  bwp::steam::SteamWorkshopClient::getInstance().search(
      query, 1, [this](const std::vector<bwp::steam::WorkshopItem> &items) {
        // Update UI on main thread
        g_idle_add(
            [](gpointer data) -> gboolean {
              auto *pair = static_cast<std::pair<
                  WorkshopView *, std::vector<bwp::steam::WorkshopItem>> *>(
                  data);
              pair->first->updateGrid(pair->second);
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<WorkshopView *,
                          std::vector<bwp::steam::WorkshopItem>>(this, items));
      });
}

void WorkshopView::updateGrid(
    const std::vector<bwp::steam::WorkshopItem> &items) {
  // Clear grid
  GtkWidget *child = gtk_widget_get_first_child(m_grid);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(m_grid), child);
    child = next;
  }

  for (const auto &item : items) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_add_css_class(card, "card");
    gtk_widget_set_size_request(card, 200, 200);

    // Mock image (label for now)
    GtkWidget *imgLabel = gtk_label_new(item.title.c_str());
    gtk_widget_set_vexpand(imgLabel, TRUE);
    gtk_box_append(GTK_BOX(card), imgLabel);

    GtkWidget *downloadBtn = gtk_button_new_with_label("Download");
    gtk_widget_set_margin_bottom(downloadBtn, 5);

    ItemData *data = new ItemData{item.id, this};
    g_object_set_data_full(
        G_OBJECT(downloadBtn), "item-data", data,
        [](gpointer d) { delete static_cast<ItemData *>(d); });

    g_signal_connect(downloadBtn, "clicked", G_CALLBACK(onDownloadClicked),
                     data);
    gtk_box_append(GTK_BOX(card), downloadBtn);

    gtk_flow_box_insert(GTK_FLOW_BOX(m_grid), card, -1);
  }
}

void WorkshopView::onDownloadClicked(GtkButton *button, gpointer user_data) {
  ItemData *data = static_cast<ItemData *>(user_data);
  WorkshopView *self = data->view;

  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(self->m_progressBar));

  bwp::steam::SteamWorkshopClient::getInstance().download(
      data->id,
      [self](double p) {
        // progress update
      },
      [self](bool success, const std::string &path) {
        g_idle_add(
            [](gpointer d) -> gboolean {
              auto *s = static_cast<WorkshopView *>(d);
              gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(s->m_progressBar),
                                            1.0);
              // Trigger library scan or add
              return G_SOURCE_REMOVE;
            },
            self);
      });
}

} // namespace bwp::gui
