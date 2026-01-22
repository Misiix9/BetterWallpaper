#include "WallpaperGrid.hpp"
#include "../../core/utils/StringUtils.hpp"
#include <iostream>

namespace bwp::gui {

WallpaperGrid::WallpaperGrid() {
  m_scrolledWindow = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(m_scrolledWindow, TRUE);
  gtk_widget_set_hexpand(m_scrolledWindow, TRUE);

  m_flowBox = gtk_flow_box_new();
  gtk_flow_box_set_valign(GTK_FLOW_BOX(m_flowBox), GTK_ALIGN_START);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_flowBox),
                                         10); // Dynamic usually
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_flowBox), 1);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_flowBox),
                                  GTK_SELECTION_SINGLE);
  gtk_widget_set_margin_start(m_flowBox, 12);
  gtk_widget_set_margin_end(m_flowBox, 12);
  gtk_widget_set_margin_bottom(m_flowBox, 12);

  // Spacing
  gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(m_flowBox), 12);
  gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(m_flowBox), 12);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                m_flowBox);
}

WallpaperGrid::~WallpaperGrid() {
  // Widgets destroyed by parent
}

void WallpaperGrid::clear() {
  // Remove all children
  GtkWidget *child = gtk_widget_get_first_child(m_flowBox);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(m_flowBox), child);
    child = next;
  }
  m_cards.clear();
  m_items.clear();
}

void WallpaperGrid::addWallpaper(const bwp::wallpaper::WallpaperInfo &info) {
  auto card = std::make_unique<WallpaperCard>(info);
  card->updateThumbnail(info.path);

  gtk_flow_box_append(GTK_FLOW_BOX(m_flowBox), card->getWidget());

  Item item;
  item.info = info;
  item.card = card.get();
  m_items.push_back(item);

  m_cards.push_back(std::move(card));
}

void WallpaperGrid::filter(const std::string &query) {
  std::string q = utils::StringUtils::toLower(utils::StringUtils::trim(query));

  // GtkFlowBox has set_filter_func but it requires C callback.
  // Iterating widgets to show/hide is alternative.
  // Let's use set_filter_func approach properly or simple loop.
  // Simple loop for now:

  for (const auto &item : m_items) {
    bool visible = true;
    if (!q.empty()) {
      std::string title = std::filesystem::path(item.info.path).stem().string();
      // Basic search
      if (utils::StringUtils::toLower(title).find(q) == std::string::npos) {
        visible = false;
      }
    }

    // Find parent GtkFlowBoxChild to hide?
    // getWidget returns the card box content.
    GtkWidget *parent =
        gtk_widget_get_parent(item.card->getWidget()); // This is FlowBoxChild
    if (parent) {
      gtk_widget_set_visible(parent, visible);
    }
  }
}

void WallpaperGrid::setSortOrder(int sortInfo) {
  // GtkFlowBox set_sort_func
  // Todo
}

} // namespace bwp::gui
