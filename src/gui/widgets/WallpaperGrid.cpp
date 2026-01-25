#include "WallpaperGrid.hpp"
#include "../../core/utils/StringUtils.hpp"
#include <filesystem>

namespace bwp::gui {

WallpaperGrid::WallpaperGrid() {
  m_scrolledWindow = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(m_scrolledWindow, TRUE);
  gtk_widget_set_hexpand(m_scrolledWindow, TRUE);

  m_flowBox = gtk_flow_box_new();
  gtk_widget_set_valign(m_flowBox, GTK_ALIGN_START);
  gtk_widget_set_halign(m_flowBox, GTK_ALIGN_FILL);

  // Force multiple columns
  // homogenous=TRUE is required for a clean grid layout where items align
  gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(m_flowBox), TRUE);

  // Set constraints to force at least 4 columns as requested
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_flowBox), 4);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_flowBox), 12);

  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_flowBox),
                                  GTK_SELECTION_SINGLE);

  gtk_widget_set_margin_start(m_flowBox, 16);
  gtk_widget_set_margin_end(m_flowBox, 16);
  gtk_widget_set_margin_top(m_flowBox, 16);
  gtk_widget_set_margin_bottom(m_flowBox, 16);

  gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(m_flowBox), 16);
  gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(m_flowBox), 16);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrolledWindow),
                                m_flowBox);

  // Selection callback
  g_signal_connect(
      m_flowBox, "child-activated",
      G_CALLBACK(+[](GtkFlowBox *, GtkFlowBoxChild *child, gpointer data) {
        WallpaperGrid *self = static_cast<WallpaperGrid *>(data);
        if (self->m_callback) {
          GtkWidget *cardWidget = gtk_flow_box_child_get_child(child);
          if (cardWidget) {
            bwp::wallpaper::WallpaperInfo *info =
                static_cast<bwp::wallpaper::WallpaperInfo *>(
                    g_object_get_data(G_OBJECT(cardWidget), "wallpaper-info"));
            if (info) {
              self->m_callback(*info);
            }
          }
        }
      }),
      this);
}

void WallpaperGrid::setSelectionCallback(SelectionCallback callback) {
  m_callback = callback;
}

WallpaperGrid::~WallpaperGrid() {}

void WallpaperGrid::clear() {
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
  for (const auto &item : m_items) {
    if (item.info.id == info.id)
      return;
  }

  auto card = std::make_unique<WallpaperCard>(info);
  card->updateThumbnail(info.path);

  // Ensure the card widget itself is centered in the allocation
  gtk_widget_set_halign(card->getWidget(), GTK_ALIGN_CENTER);
  gtk_widget_set_valign(card->getWidget(), GTK_ALIGN_CENTER);

  g_object_set_data_full(
      G_OBJECT(card->getWidget()), "wallpaper-info",
      new bwp::wallpaper::WallpaperInfo(info), [](gpointer data) {
        delete static_cast<bwp::wallpaper::WallpaperInfo *>(data);
      });

  gtk_flow_box_append(GTK_FLOW_BOX(m_flowBox), card->getWidget());

  Item item;
  item.info = info;
  item.card = card.get();
  m_items.push_back(item);

  m_cards.push_back(std::move(card));
}

void WallpaperGrid::filter(const std::string &query) {
  std::string q = utils::StringUtils::toLower(utils::StringUtils::trim(query));

  for (const auto &item : m_items) {
    bool visible = true;
    if (!q.empty()) {
      std::string title = std::filesystem::path(item.info.path).stem().string();
      if (utils::StringUtils::toLower(title).find(q) == std::string::npos) {
        visible = false;
      }
    }

    GtkWidget *parent = gtk_widget_get_parent(item.card->getWidget());
    if (parent) {
      gtk_widget_set_visible(parent, visible);
    }
  }
}

void WallpaperGrid::setSortOrder(int /*sortInfo*/) {}

} // namespace bwp::gui
