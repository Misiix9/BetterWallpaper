#include "WorkshopView.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include <iostream>

namespace bwp::gui {

WorkshopView::WorkshopView() { setupUi(); }

WorkshopView::~WorkshopView() {}

void WorkshopView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Tab Switcher
  m_stack = adw_view_stack_new();
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher),
                              ADW_VIEW_STACK(m_stack));
  adw_view_switcher_set_policy(ADW_VIEW_SWITCHER(switcher),
                               ADW_VIEW_SWITCHER_POLICY_WIDE);

  gtk_box_append(GTK_BOX(m_box), switcher);
  gtk_widget_set_vexpand(m_stack, TRUE);
  gtk_box_append(GTK_BOX(m_box), m_stack);

  setupInstalledPage();
  setupBrowsePage();

  // Defer network call to after window is shown
  // performSearch("anime"); // REMOVED - causes blocking
}

void WorkshopView::setupInstalledPage() {
  m_installedPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  m_installedGrid = std::make_unique<WallpaperGrid>();
  gtk_box_append(GTK_BOX(m_installedPage), m_installedGrid->getWidget());

  // Defer library loading to idle callback
  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<WorkshopView *>(data);
        self->refreshInstalled();
        return G_SOURCE_REMOVE;
      },
      this);

  adw_view_stack_add_titled(ADW_VIEW_STACK(m_stack), m_installedPage,
                            "installed", "Installed");
}

void WorkshopView::refreshInstalled() {
  if (!m_installedGrid)
    return;
  m_installedGrid->clear();

  auto &lib = bwp::wallpaper::WallpaperLibrary::getInstance();
  auto workshopItems =
      lib.filter([](const bwp::wallpaper::WallpaperInfo &info) {
        return info.source == "workshop" ||
               info.type == bwp::wallpaper::WallpaperType::WEScene ||
               info.type == bwp::wallpaper::WallpaperType::WEVideo;
      });

  for (const auto &info : workshopItems) {
    m_installedGrid->addWallpaper(info);
  }
}

void WorkshopView::setupBrowsePage() {
  m_browsePage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(m_browsePage, 10);
  gtk_widget_set_margin_end(m_browsePage, 10);
  gtk_widget_set_margin_top(m_browsePage, 10);
  gtk_widget_set_margin_bottom(m_browsePage, 10);

  // Header for Search
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(m_browsePage), headerBox);

  GtkWidget *title = gtk_label_new("Browse Steam Workshop");
  gtk_widget_add_css_class(title, "heading");
  gtk_box_append(GTK_BOX(headerBox), title);

  m_searchBar = gtk_search_entry_new();
  gtk_widget_set_hexpand(m_searchBar, TRUE);
  g_signal_connect(m_searchBar, "activate", G_CALLBACK(onSearchEnter), this);
  gtk_box_append(GTK_BOX(headerBox), m_searchBar);

  // Progress
  m_progressBar = gtk_progress_bar_new();
  gtk_box_append(GTK_BOX(m_browsePage), m_progressBar);

  // Scrolled Grid
  m_browseScrolled = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(m_browseScrolled, TRUE);
  gtk_box_append(GTK_BOX(m_browsePage), m_browseScrolled);

  m_browseGrid = gtk_flow_box_new();
  gtk_widget_set_valign(m_browseGrid, GTK_ALIGN_START);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_browseGrid), 30);
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_browseGrid), 1);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_browseGrid),
                                  GTK_SELECTION_NONE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_browseScrolled),
                                m_browseGrid);

  adw_view_stack_add_titled(ADW_VIEW_STACK(m_stack), m_browsePage, "browse",
                            "Browse");
}

void WorkshopView::onSearchEnter(GtkEntry *entry, gpointer user_data) {
  auto *self = static_cast<WorkshopView *>(user_data);
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
  self->performSearch(text);
}

void WorkshopView::performSearch(const std::string &query) {
  bwp::steam::SteamWorkshopClient::getInstance().search(
      query, 1, [this](const std::vector<bwp::steam::WorkshopItem> &items) {
        g_idle_add(
            [](gpointer data) -> gboolean {
              auto *pair = static_cast<std::pair<
                  WorkshopView *, std::vector<bwp::steam::WorkshopItem>> *>(
                  data);
              pair->first->updateBrowseGrid(pair->second);
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<WorkshopView *,
                          std::vector<bwp::steam::WorkshopItem>>(this, items));
      });
}

void WorkshopView::updateBrowseGrid(
    const std::vector<bwp::steam::WorkshopItem> &items) {
  GtkWidget *child = gtk_widget_get_first_child(m_browseGrid);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(m_browseGrid), child);
    child = next;
  }

  for (const auto &item : items) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_add_css_class(card, "card");
    gtk_widget_set_size_request(card, 200, 200);

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

    gtk_flow_box_insert(GTK_FLOW_BOX(m_browseGrid), card, -1);
  }
}

void WorkshopView::onDownloadClicked(GtkButton * /*button*/,
                                     gpointer user_data) {
  ItemData *data = static_cast<ItemData *>(user_data);
  WorkshopView *self = data->view;

  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(self->m_progressBar));

  bwp::steam::SteamWorkshopClient::getInstance().download(
      data->id,
      [self](const bwp::steam::DownloadProgress &prog) {
        // Update progress bar from main thread
        g_idle_add(
            [](gpointer d) -> gboolean {
              auto *pair = static_cast<std::pair<WorkshopView *, double> *>(d);
              gtk_progress_bar_set_fraction(
                  GTK_PROGRESS_BAR(pair->first->m_progressBar), pair->second);
              delete pair;
              return G_SOURCE_REMOVE;
            },
            new std::pair<WorkshopView *, double>(self, prog.progress));
      },
      [self](bool /*success*/, const std::string & /*path*/) {
        g_idle_add(
            [](gpointer d) -> gboolean {
              auto *s = static_cast<WorkshopView *>(d);
              gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(s->m_progressBar),
                                            1.0);
              s->refreshInstalled();
              return G_SOURCE_REMOVE;
            },
            self);
      });
}

} // namespace bwp::gui
