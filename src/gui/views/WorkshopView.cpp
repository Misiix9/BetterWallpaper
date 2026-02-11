#include "WorkshopView.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/WallpaperLibrary.hpp"
#include "../../core/steam/SteamService.hpp"
#include "../../core/steam/DownloadQueue.hpp"
#include "../../core/utils/ToastManager.hpp"
#include <curl/curl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <iostream>
#include <thread>

namespace bwp::gui {

WorkshopView::WorkshopView() {
  setupUi();
  setupDownloadCallbacks();
}
WorkshopView::~WorkshopView() {
  dismissDownloadToast();
}

void WorkshopView::setupUi() {
  m_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  
  // Directly show Browse Page (No tabs/switcher)
  setupBrowsePage();
  gtk_widget_set_vexpand(m_browsePage, TRUE);
  gtk_box_append(GTK_BOX(m_box), m_browsePage);
}
void WorkshopView::setupInstalledPage() {
  // Deprecated / Unused in new layout
  m_installedPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
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
  m_steamcmdBanner =
      adw_banner_new("steamcmd is required for downloading Workshop content");
  adw_banner_set_button_label(ADW_BANNER(m_steamcmdBanner), "How to Install");
  adw_banner_set_revealed(ADW_BANNER(m_steamcmdBanner), false);
  g_signal_connect(m_steamcmdBanner, "button-clicked",
                   G_CALLBACK(+[](AdwBanner *, gpointer user_data) {
                     auto *self = static_cast<WorkshopView *>(user_data);
                     self->showSteamcmdInstallDialog();
                   }),
                   this);
  gtk_box_append(GTK_BOX(m_browsePage), m_steamcmdBanner);
  if (!bwp::steam::SteamService::getInstance().hasSteamCMD()) {
    adw_banner_set_revealed(ADW_BANNER(m_steamcmdBanner), true);
  }
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(m_browsePage), headerBox);
  GtkWidget *title = gtk_label_new("Browse Steam Workshop");
  gtk_widget_add_css_class(title, "heading");
  gtk_box_append(GTK_BOX(headerBox), title);
  m_searchBar = gtk_search_entry_new();
  gtk_widget_set_hexpand(m_searchBar, TRUE);
  gtk_widget_add_css_class(m_searchBar, "workshop-search-bar");
  g_signal_connect(m_searchBar, "activate", G_CALLBACK(onSearchEnter), this);
  gtk_box_append(GTK_BOX(headerBox), m_searchBar);
  m_loginButton = gtk_button_new();
  gtk_widget_add_css_class(m_loginButton, "workshop-user-btn");
  gtk_widget_set_tooltip_text(
      m_loginButton,
      "Log in to Steam for access to more wallpapers (optional)");
  g_signal_connect(
      m_loginButton, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer user_data) {
        auto *self = static_cast<WorkshopView *>(user_data);
        if (!bwp::steam::SteamService::getInstance().getSteamUser().empty()) {
          self->showProfileMenu();
        } else {
          self->showSteamLoginDialog();
        }
      }),
      this);
  gtk_box_append(GTK_BOX(headerBox), m_loginButton);
  m_progressBar = gtk_progress_bar_new();
  gtk_widget_set_visible(m_progressBar, FALSE);
  gtk_box_append(GTK_BOX(m_browsePage), m_progressBar);
  m_browseScrolled = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(m_browseScrolled, TRUE);
  gtk_box_append(GTK_BOX(m_browsePage), m_browseScrolled);
  m_browseGrid = gtk_flow_box_new();
  gtk_widget_set_valign(m_browseGrid, GTK_ALIGN_START);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_browseGrid), 4);
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_browseGrid), 2);
  gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(m_browseGrid), TRUE);
  gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(m_browseGrid), 8);
  gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(m_browseGrid), 8);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_browseGrid),
                                  GTK_SELECTION_NONE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_browseScrolled),
                                m_browseGrid);

  // Pagination bar
  m_paginationBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(m_paginationBox, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(m_paginationBox, 8);
  gtk_widget_set_margin_bottom(m_paginationBox, 8);
  gtk_widget_set_visible(m_paginationBox, FALSE);

  m_prevButton = gtk_button_new_from_icon_name("go-previous-symbolic");
  gtk_widget_add_css_class(m_prevButton, "flat");
  gtk_widget_set_sensitive(m_prevButton, FALSE);
  g_signal_connect(m_prevButton, "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer data) {
          auto* self = static_cast<WorkshopView*>(data);
          if (self->m_currentPage > 1)
              self->performSearch(self->m_lastQuery, self->m_currentPage - 1, self->m_lastSort);
      }), this);
  gtk_box_append(GTK_BOX(m_paginationBox), m_prevButton);

  m_pageLabel = gtk_label_new("Page 1 / 1");
  gtk_widget_add_css_class(m_pageLabel, "dim-label");
  gtk_box_append(GTK_BOX(m_paginationBox), m_pageLabel);

  m_nextButton = gtk_button_new_from_icon_name("go-next-symbolic");
  gtk_widget_add_css_class(m_nextButton, "flat");
  gtk_widget_set_sensitive(m_nextButton, FALSE);
  g_signal_connect(m_nextButton, "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer data) {
          auto* self = static_cast<WorkshopView*>(data);
          if (self->m_currentPage < self->m_totalPages)
              self->performSearch(self->m_lastQuery, self->m_currentPage + 1, self->m_lastSort);
      }), this);
  gtk_box_append(GTK_BOX(m_paginationBox), m_nextButton);

  gtk_box_append(GTK_BOX(m_browsePage), m_paginationBox);

  // Placeholder for logged-out state
  m_placeholderBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_vexpand(m_placeholderBox, TRUE);
  gtk_widget_set_hexpand(m_placeholderBox, TRUE);
  gtk_widget_set_valign(m_placeholderBox, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(m_placeholderBox, GTK_ALIGN_CENTER);

  GtkWidget* placeholderIcon = gtk_image_new_from_icon_name("system-users-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(placeholderIcon), 64);
  gtk_widget_add_css_class(placeholderIcon, "dim-label");
  gtk_box_append(GTK_BOX(m_placeholderBox), placeholderIcon);

  GtkWidget* placeholderTitle = gtk_label_new("Steam Workshop");
  gtk_widget_add_css_class(placeholderTitle, "title-1");
  gtk_widget_add_css_class(placeholderTitle, "dim-label");
  gtk_box_append(GTK_BOX(m_placeholderBox), placeholderTitle);

  GtkWidget* placeholderDesc = gtk_label_new("Log in to your Steam account to browse\nand download Workshop wallpapers.");
  gtk_label_set_justify(GTK_LABEL(placeholderDesc), GTK_JUSTIFY_CENTER);
  gtk_widget_add_css_class(placeholderDesc, "dim-label");
  gtk_box_append(GTK_BOX(m_placeholderBox), placeholderDesc);

  gtk_box_append(GTK_BOX(m_browsePage), m_placeholderBox);

  // Initial state: show placeholder or load popular wallpapers
  refreshLoginState();
}
void WorkshopView::onSearchEnter(GtkEntry *entry, gpointer user_data) {
  auto *self = static_cast<WorkshopView *>(user_data);
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
  self->performSearch(text);
}
void WorkshopView::performSearch(const std::string &query, int page, const std::string &sort) {
  // Check for API Key first
  if (bwp::steam::SteamService::getInstance().getApiKey().empty()) {
      showApiKeyDialog();
      return;
  }
  
  if (m_isSearching) return;
  m_isSearching = true;
  m_lastQuery = query;
  m_lastSort = sort;

  // Hide placeholder, show browse area
  gtk_widget_set_visible(m_placeholderBox, FALSE);
  gtk_widget_set_visible(m_browseScrolled, TRUE);

  // Show progress (do not disable search bar to avoid GtkText focus warning)
  gtk_widget_set_visible(m_progressBar, TRUE);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(m_progressBar));
  
  // Scroll to top
  GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(m_browseScrolled));
  gtk_adjustment_set_value(vadj, 0);
  
  struct SearchData {
      WorkshopView* view;
      bwp::steam::SearchResult result;
      int page;
  };
  
  std::thread([this, query, page, sort]() {
      auto result = bwp::steam::SteamService::getInstance().search(query, page, sort);
      auto* data = new SearchData{this, std::move(result), page};
      g_idle_add(
          [](gpointer gdata) -> gboolean {
            auto *d = static_cast<SearchData*>(gdata);
            d->view->updateBrowseGrid(d->result.items);
            
            // Update pagination state
            d->view->m_currentPage = d->page;
            int total = d->result.total;
            d->view->m_totalPages = (total > 0) ? ((total + 49) / 50) : 1;
            d->view->updatePagination();
            
            d->view->m_isSearching = false;
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(d->view->m_progressBar), 0.0);
            gtk_widget_set_visible(d->view->m_progressBar, FALSE);
            
            delete d;
            return G_SOURCE_REMOVE;
          }, data);
  }).detach();
}
void WorkshopView::updateBrowseGrid(
    const std::vector<bwp::steam::WorkshopItem> &items) {
  GtkWidget *child = gtk_widget_get_first_child(m_browseGrid);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_flow_box_remove(GTK_FLOW_BOX(m_browseGrid), child);
    child = next;
  }

  static constexpr int CARD_WIDTH = 180;
  static constexpr int CARD_HEIGHT = 180;

  for (const auto &item : items) {
    // Main card box — same as library cards (4:3 ratio)
    GtkWidget *cardBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(cardBox, CARD_WIDTH, CARD_HEIGHT);
    gtk_widget_set_hexpand(cardBox, TRUE);
    gtk_widget_add_css_class(cardBox, "wallpaper-card");

    // Overlay for image + title + download btn
    GtkWidget *overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(overlay, TRUE);
    gtk_widget_set_vexpand(overlay, TRUE);
    gtk_widget_set_overflow(overlay, GTK_OVERFLOW_HIDDEN);
    gtk_box_append(GTK_BOX(cardBox), overlay);

    // Image
    GtkWidget *image = gtk_picture_new();
    gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_COVER);
    gtk_widget_set_size_request(image, CARD_WIDTH, CARD_HEIGHT);
    gtk_widget_set_hexpand(image, TRUE);
    gtk_widget_set_vexpand(image, TRUE);
    gtk_widget_add_css_class(image, "card-image");
    gtk_widget_add_css_class(image, "no-thumbnail");
    gtk_overlay_set_child(GTK_OVERLAY(overlay), image);

    // Skeleton loading overlay
    GtkWidget *skeleton = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(skeleton, "skeleton");
    gtk_widget_set_halign(skeleton, GTK_ALIGN_FILL);
    gtk_widget_set_valign(skeleton, GTK_ALIGN_FILL);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), skeleton);

    // Title + download button container at bottom
    GtkWidget *bottomBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(bottomBar, "card-title-overlay");
    gtk_widget_set_valign(bottomBar, GTK_ALIGN_END);
    gtk_widget_set_halign(bottomBar, GTK_ALIGN_FILL);

    // Title label (left)
    GtkWidget *titleLabel = gtk_label_new(item.title.c_str());
    gtk_label_set_ellipsize(GTK_LABEL(titleLabel), PANGO_ELLIPSIZE_END);
    gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
    gtk_widget_set_hexpand(titleLabel, TRUE);
    gtk_widget_add_css_class(titleLabel, "card-title-text");
    gtk_box_append(GTK_BOX(bottomBar), titleLabel);

    // Download button (right)
    GtkWidget *downloadBtn = gtk_button_new_from_icon_name("folder-download-symbolic");
    gtk_widget_add_css_class(downloadBtn, "flat");
    gtk_widget_add_css_class(downloadBtn, "card-fav-btn"); // reuse same small btn style
    gtk_widget_set_halign(downloadBtn, GTK_ALIGN_END);
    gtk_widget_set_valign(downloadBtn, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(downloadBtn, "Download");

    ItemData *data = new ItemData{item.id, item.title, this};
    g_object_set_data_full(
        G_OBJECT(downloadBtn), "item-data", data,
        [](gpointer d) { delete static_cast<ItemData *>(d); });
    g_signal_connect(downloadBtn, "clicked", G_CALLBACK(onDownloadClicked), data);
    gtk_box_append(GTK_BOX(bottomBar), downloadBtn);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), bottomBar);

    gtk_flow_box_insert(GTK_FLOW_BOX(m_browseGrid), cardBox, -1);

    // Constrain the FlowBoxChild wrapper to not stretch the card
    GtkWidget *fbChild = gtk_widget_get_parent(cardBox);
    if (fbChild) {
      gtk_widget_set_halign(fbChild, GTK_ALIGN_START);
      gtk_widget_set_valign(fbChild, GTK_ALIGN_START);
    }

    // Async thumbnail download
    if (!item.thumbnailUrl.empty()) {
      struct ThumbData {
        GtkWidget *image;
        GtkWidget *skeleton;
        std::string url;
        std::shared_ptr<bool> alive;
      };
      auto alive = std::make_shared<bool>(true);
      g_object_set_data_full(G_OBJECT(image), "alive-token",
          new std::shared_ptr<bool>(alive),
          [](gpointer p) { delete static_cast<std::shared_ptr<bool>*>(p); });

      auto *td = new ThumbData{image, skeleton, item.thumbnailUrl, alive};
      std::thread([td]() {
        // Download image bytes via curl
        CURL *curl = curl_easy_init();
        std::string imgData;
        if (curl) {
          curl_easy_setopt(curl, CURLOPT_URL, td->url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
              +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
                static_cast<std::string*>(userp)->append(
                    static_cast<char*>(contents), size * nmemb);
                return size * nmemb;
              });
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imgData);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "BetterWallpaper/1.0");
          curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
          curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
          CURLcode res = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
          if (res != CURLE_OK) {
            delete td;
            return;
          }
        }

        // Create texture from image bytes
        GBytes *bytes = g_bytes_new(imgData.data(), imgData.size());
        GInputStream *stream = g_memory_input_stream_new_from_bytes(bytes);
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
        g_object_unref(stream);
        g_bytes_unref(bytes);

        if (!pixbuf) {
          delete td;
          return;
        }

        struct IdleData {
          GdkPixbuf *pixbuf;
          GtkWidget *image;
          GtkWidget *skeleton;
          std::shared_ptr<bool> alive;
        };
        auto *id = new IdleData{pixbuf, td->image, td->skeleton, td->alive};
        delete td;

        g_idle_add(+[](gpointer data) -> gboolean {
          auto *d = static_cast<IdleData*>(data);
          if (*d->alive) {
            GdkTexture *texture = GDK_TEXTURE(
                gdk_memory_texture_new(
                    gdk_pixbuf_get_width(d->pixbuf),
                    gdk_pixbuf_get_height(d->pixbuf),
                    gdk_pixbuf_get_has_alpha(d->pixbuf) ? GDK_MEMORY_R8G8B8A8 : GDK_MEMORY_R8G8B8,
                    g_bytes_new_with_free_func(
                        gdk_pixbuf_get_pixels(d->pixbuf),
                        gdk_pixbuf_get_height(d->pixbuf) * gdk_pixbuf_get_rowstride(d->pixbuf),
                        (GDestroyNotify)g_object_unref, g_object_ref(d->pixbuf)),
                    gdk_pixbuf_get_rowstride(d->pixbuf)));
            gtk_picture_set_paintable(GTK_PICTURE(d->image), GDK_PAINTABLE(texture));
            gtk_widget_remove_css_class(d->image, "no-thumbnail");
            gtk_widget_set_visible(d->skeleton, FALSE);
            g_object_unref(texture);
          }
          g_object_unref(d->pixbuf);
          delete d;
          return G_SOURCE_REMOVE;
        }, id);
      }).detach();
    } else {
      gtk_widget_set_visible(skeleton, FALSE);
    }
  }
}
void WorkshopView::onDownloadClicked(GtkButton *  ,
                                     gpointer user_data) {
  ItemData *data = static_cast<ItemData *>(user_data);
  std::string title = data->title.empty() ? data->id : data->title;
  
  bwp::steam::DownloadQueue::getInstance().addToQueue(data->id, title);
  
  // Show initial persistent download toast (0%)
  data->view->showDownloadProgress(title, 0.0f);
}

void WorkshopView::updatePagination() {
  if (!m_paginationBox) return;

  bool hasResults = m_totalPages > 0;
  gtk_widget_set_visible(m_paginationBox, hasResults);

  if (hasResults) {
    std::string label = "Page " + std::to_string(m_currentPage) + " / " + std::to_string(m_totalPages);
    gtk_label_set_text(GTK_LABEL(m_pageLabel), label.c_str());
    gtk_widget_set_sensitive(m_prevButton, m_currentPage > 1);
    gtk_widget_set_sensitive(m_nextButton, m_currentPage < m_totalPages);
  }
}

void WorkshopView::loadPopularWallpapers() {
  // Only load if logged in and API key is available
  auto& steam = bwp::steam::SteamService::getInstance();
  if (steam.getSteamUser().empty() || steam.getApiKey().empty()) return;

  // Search with empty query and ranked sort = popular wallpapers
  performSearch("", 1, "ranked");
}

void WorkshopView::showSteamLoginDialog(const std::string& prefillUser,
                                        const std::string& errorMsg) {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

  auto *dialog = adw_alert_dialog_new("Steam Login",
      "Enter your Steam credentials to enable downloads.\n"
      "2FA will be prompted if enabled on your account.");
  
  GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  
  GtkWidget* userEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(userEntry), "Username");
  if (!prefillUser.empty())
    gtk_editable_set_text(GTK_EDITABLE(userEntry), prefillUser.c_str());
  gtk_box_append(GTK_BOX(content), userEntry);

  GtkWidget* passEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(passEntry), "Password");
  gtk_entry_set_visibility(GTK_ENTRY(passEntry), FALSE);
  gtk_box_append(GTK_BOX(content), passEntry);

  // Error label — visible if errorMsg was provided (reopen-on-failure pattern)
  GtkWidget* errorLabel = gtk_label_new(nullptr);
  gtk_widget_add_css_class(errorLabel, "error");
  gtk_label_set_wrap(GTK_LABEL(errorLabel), TRUE);
  if (!errorMsg.empty()) {
    gtk_label_set_text(GTK_LABEL(errorLabel), errorMsg.c_str());
    gtk_widget_set_visible(errorLabel, TRUE);
  } else {
    gtk_widget_set_visible(errorLabel, FALSE);
  }
  gtk_box_append(GTK_BOX(content), errorLabel);

  adw_alert_dialog_set_extra_child(ADW_ALERT_DIALOG(dialog), content);
  
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", "Cancel");
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "login", "Login");
  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "login", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "login");

  // Store entry pointers for reading in the response handler.
  // AdwAlertDialog auto-closes on ANY response click, so we must read entries
  // synchronously in the handler and never access the dialog from async callbacks.
  g_object_set_data(G_OBJECT(dialog), "user_entry", userEntry);
  g_object_set_data(G_OBJECT(dialog), "pass_entry", passEntry);

  auto loginResponseCb = +[](AdwAlertDialog *d, const char *response, gpointer selfPtr) {
      auto* self = static_cast<WorkshopView*>(selfPtr);

      if (g_strcmp0(response, "login") != 0) return; // Cancel — dialog closes, done

      // Read entries NOW — dialog is still alive during the signal handler
      GtkWidget* u = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "user_entry"));
      GtkWidget* p = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "pass_entry"));
      std::string user = gtk_editable_get_text(GTK_EDITABLE(u));
      std::string pass = gtk_editable_get_text(GTK_EDITABLE(p));
      // From here on, only use `self`, `user`, `pass` (safe value copies).
      // The dialog will be destroyed once this handler returns.

      if (user.empty() || pass.empty()) {
          // Dialog is closing — reopen with error after stack unwinds
          struct ReopenData { WorkshopView* view; std::string user; };
          auto* rd = new ReopenData{self, user};
          g_idle_add(+[](gpointer data) -> gboolean {
              auto* d = static_cast<ReopenData*>(data);
              d->view->showSteamLoginDialog(d->user, "Username and password are required.");
              delete d;
              return G_SOURCE_REMOVE;
          }, rd);
          return;
      }

      bwp::steam::SteamService::getInstance().setSteamUser(user);

      struct LoginResult {
          WorkshopView* view;
          std::string user;
          bool success;
          std::string msg;
      };

      bwp::steam::SteamService::getInstance().login(pass,
          // Login result callback (background thread → g_idle_add)
          [self, user](bool success, const std::string& msg) {
              auto* r = new LoginResult{self, user, success, msg};
              g_idle_add(+[](gpointer data) -> gboolean {
                  auto* r = static_cast<LoginResult*>(data);
                  if (r->success) {
                      r->view->refreshLoginState();

                      bwp::core::utils::ToastRequest req;
                      req.message = "Steam login successful!";
                      req.type = bwp::core::utils::ToastType::Success;
                      bwp::core::utils::ToastManager::getInstance().showToast(req);

                      if (bwp::steam::SteamService::getInstance().getApiKey().empty())
                          r->view->showApiKeyDialog();
                  } else {
                      // Clear user and reopen dialog with error + username prefilled
                      bwp::steam::SteamService::getInstance().setSteamUser("");
                      r->view->showSteamLoginDialog(r->user, r->msg);
                  }
                  delete r;
                  return G_SOURCE_REMOVE;
              }, r);
          },
          // 2FA needed callback
          [self]() {
              g_idle_add(+[](gpointer data) -> gboolean {
                  static_cast<WorkshopView*>(data)->showSteam2FADialog();
                  return G_SOURCE_REMOVE;
              }, self);
          });
      // Dialog auto-closes when handler returns; login continues in background
  };
  g_signal_connect(dialog, "response", G_CALLBACK(loginResponseCb), this);

  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

void WorkshopView::showSteam2FADialog(const std::string& errorMsg) {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  
  auto *dialog = adw_alert_dialog_new("Steam Guard",
      "Enter the 2FA code from your email or mobile authenticator.");
  
  GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget* codeEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(codeEntry), "XXXXX");
  gtk_box_append(GTK_BOX(content), codeEntry);

  // Error label — visible if errorMsg was provided (reopen-on-failure pattern)
  GtkWidget* errorLabel = gtk_label_new(nullptr);
  gtk_widget_add_css_class(errorLabel, "error");
  gtk_label_set_wrap(GTK_LABEL(errorLabel), TRUE);
  if (!errorMsg.empty()) {
    gtk_label_set_text(GTK_LABEL(errorLabel), errorMsg.c_str());
    gtk_widget_set_visible(errorLabel, TRUE);
  } else {
    gtk_widget_set_visible(errorLabel, FALSE);
  }
  gtk_box_append(GTK_BOX(content), errorLabel);

  adw_alert_dialog_set_extra_child(ADW_ALERT_DIALOG(dialog), content);

  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", "Cancel");
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "submit", "Submit");
  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "submit", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "submit");
  
  g_object_set_data(G_OBJECT(dialog), "code_entry", codeEntry);
  
  auto twoFAResponseCb = +[](AdwAlertDialog *d, const char *response, gpointer selfPtr) {
      auto* self = static_cast<WorkshopView*>(selfPtr);

      if (g_strcmp0(response, "submit") != 0) return; // Cancel — done

      // Read entry NOW — dialog is still alive during the signal handler
      GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "code_entry"));
      std::string code = gtk_editable_get_text(GTK_EDITABLE(entry));
      // Don't access dialog after this point

      if (code.empty()) {
          g_idle_add(+[](gpointer data) -> gboolean {
              static_cast<WorkshopView*>(data)->showSteam2FADialog("Please enter the 2FA code.");
              return G_SOURCE_REMOVE;
          }, self);
          return;
      }

      struct TwoFAResult {
          WorkshopView* view;
          bool success;
          std::string msg;
      };

      bwp::steam::SteamService::getInstance().submit2FA(code,
          [self](bool success, const std::string& msg) {
              auto* r = new TwoFAResult{self, success, msg};
              g_idle_add(+[](gpointer data) -> gboolean {
                  auto* r = static_cast<TwoFAResult*>(data);
                  if (r->success) {
                      r->view->refreshLoginState();

                      bwp::core::utils::ToastRequest req;
                      req.message = "Steam login successful!";
                      req.type = bwp::core::utils::ToastType::Success;
                      bwp::core::utils::ToastManager::getInstance().showToast(req);

                      if (bwp::steam::SteamService::getInstance().getApiKey().empty())
                          r->view->showApiKeyDialog();
                  } else {
                      // Reopen 2FA dialog with error message
                      r->view->showSteam2FADialog(r->msg);
                  }
                  delete r;
                  return G_SOURCE_REMOVE;
              }, r);
          });
      // Dialog auto-closes; 2FA verification continues in background
  };
  g_signal_connect(dialog, "response", G_CALLBACK(twoFAResponseCb), this);
  
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

void WorkshopView::showSteamcmdInstallDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  auto *dialog = adw_alert_dialog_new("Missing steamcmd", 
    "The 'steamcmd' utility is required for Workshop downloads.\n\nPlease install it using your distribution's package manager.");
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "close", "Close");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

void WorkshopView::refreshLoginState() {
  if (!m_loginButton)
    return;
  std::string user = bwp::steam::SteamService::getInstance().getSteamUser();
  bool loggedIn = !user.empty(); 

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  if (loggedIn) {
    gtk_box_append(GTK_BOX(box),
                   gtk_image_new_from_icon_name("avatar-default-symbolic"));
    GtkWidget *label = gtk_label_new(user.c_str());
    gtk_widget_add_css_class(label, "heading");
    gtk_box_append(GTK_BOX(box), label);
    gtk_widget_add_css_class(m_loginButton, "suggested-action");
    gtk_widget_set_tooltip_text(m_loginButton, "Account options");
    
    // Show browse area, hide placeholder
    if (m_placeholderBox) gtk_widget_set_visible(m_placeholderBox, FALSE);
    if (m_browseScrolled) gtk_widget_set_visible(m_browseScrolled, TRUE);
    
    // Auto-load popular wallpapers if grid is empty
    if (m_browseGrid) {
      GtkWidget* firstChild = gtk_widget_get_first_child(m_browseGrid);
      if (!firstChild) {
        loadPopularWallpapers();
      }
    }
  } else {
    gtk_box_append(GTK_BOX(box),
                   gtk_image_new_from_icon_name("system-users-symbolic"));
    gtk_box_append(GTK_BOX(box), gtk_label_new("Steam Login"));
    gtk_widget_remove_css_class(m_loginButton, "suggested-action");
    gtk_widget_set_tooltip_text(m_loginButton, "Log in to Steam");
    
    // Show placeholder, hide browse area and pagination
    if (m_placeholderBox) gtk_widget_set_visible(m_placeholderBox, TRUE);
    if (m_browseScrolled) gtk_widget_set_visible(m_browseScrolled, FALSE);
    if (m_paginationBox) gtk_widget_set_visible(m_paginationBox, FALSE);
  }
  gtk_button_set_child(GTK_BUTTON(m_loginButton), box);
}

void WorkshopView::showProfileMenu() {
  // Destroy old popover if any
  if (m_profilePopover) {
    gtk_widget_unparent(m_profilePopover);
    m_profilePopover = nullptr;
  }

  // Build menu model
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Change API Key", "workshop.change-api-key");
  g_menu_append(menu, "Log Out", "workshop.logout");

  m_profilePopover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(m_profilePopover, m_loginButton);
  gtk_popover_set_position(GTK_POPOVER(m_profilePopover), GTK_POS_BOTTOM);

  // Create action group for this popover
  GSimpleActionGroup *group = g_simple_action_group_new();

  GSimpleAction *apiKeyAction = g_simple_action_new("change-api-key", nullptr);
  g_signal_connect(apiKeyAction, "activate",
      G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
        auto *self = static_cast<WorkshopView *>(data);
        self->showApiKeyDialog();
      }), this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(apiKeyAction));
  g_object_unref(apiKeyAction);

  GSimpleAction *logoutAction = g_simple_action_new("logout", nullptr);
  g_signal_connect(logoutAction, "activate",
      G_CALLBACK(+[](GSimpleAction *, GVariant *, gpointer data) {
        auto *self = static_cast<WorkshopView *>(data);
        self->showLogoutDialog();
      }), this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(logoutAction));
  g_object_unref(logoutAction);

  gtk_widget_insert_action_group(m_profilePopover, "workshop", G_ACTION_GROUP(group));
  g_object_unref(group);
  g_object_unref(menu);

  gtk_popover_popup(GTK_POPOVER(m_profilePopover));
}

void WorkshopView::showLogoutDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
  auto *dialog = adw_alert_dialog_new("Logout",
      "This will forget your saved Steam account and API key.\nYou will need to log in again to download wallpapers.");
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", "Cancel");
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "logout", "Logout");
  adw_alert_dialog_set_response_appearance(
      ADW_ALERT_DIALOG(dialog), "logout", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "cancel");
  g_signal_connect(dialog, "response",
                   G_CALLBACK(+[](AdwAlertDialog *d, const char *response,
                                  gpointer selfPtr) {
                     auto *self = static_cast<WorkshopView *>(selfPtr);
                     if (g_strcmp0(response, "logout") == 0) {
                       // Clear all saved data
                       bwp::steam::SteamService::getInstance().setSteamUser("");
                       bwp::steam::SteamService::getInstance().setApiKey("");

                       // Clear the browse grid
                       GtkWidget *child = gtk_widget_get_first_child(self->m_browseGrid);
                       while (child) {
                         GtkWidget *next = gtk_widget_get_next_sibling(child);
                         gtk_flow_box_remove(GTK_FLOW_BOX(self->m_browseGrid), child);
                         child = next;
                       }

                       self->refreshLoginState(); // Will show placeholder and hide browse area

                       bwp::core::utils::ToastRequest req;
                       req.message = "Logged out — all saved Steam data cleared.";
                       req.type = bwp::core::utils::ToastType::Success;
                       bwp::core::utils::ToastManager::getInstance().showToast(req);
                     }
                     adw_dialog_close(ADW_DIALOG(d));
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

void WorkshopView::showApiKeyDialog() {
  GtkRoot *root = gtk_widget_get_root(m_box);
  GtkWindow *window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

  auto *dialog = adw_alert_dialog_new("Steam API Key Required", 
      "Searching the Workshop requires a valid Steam Web API Key.\n\n"
      "1. Visit https://steamcommunity.com/dev/apikey\n"
      "2. Login and copy your Key\n"
      "3. Paste it below");

  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  
  // Link Button
  GtkWidget *linkBtn = gtk_link_button_new_with_label(
      "https://steamcommunity.com/dev/apikey", "Get API Key");
  gtk_box_append(GTK_BOX(content), linkBtn);
  
  // Entry
  GtkWidget *keyEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(keyEntry), "Paste Key Here (e.g. 12345ABC...)");
  gtk_box_append(GTK_BOX(content), keyEntry);

  adw_alert_dialog_set_extra_child(ADW_ALERT_DIALOG(dialog), content);

  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", "Cancel");
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "save", "Save & Search");
  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "save", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "save");

  // Keep entry alive
  g_object_set_data(G_OBJECT(dialog), "key_entry", keyEntry);

  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *d, const char *response, gpointer selfPtr) {
       auto* self = static_cast<WorkshopView*>(selfPtr);
       if (g_strcmp0(response, "save") == 0) {
           GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(d), "key_entry"));
           std::string key = gtk_editable_get_text(GTK_EDITABLE(entry));
           
           if (!key.empty()) {
               bwp::steam::SteamService::getInstance().setApiKey(key);
               // Re-trigger search? Or just let user try again? 
               // Let's inform success.
               bwp::core::utils::ToastRequest req;
               req.message = "API Key Saved! Try searching again.";
               req.type = bwp::core::utils::ToastType::Success;
               bwp::core::utils::ToastManager::getInstance().showToast(req);
               self->refreshLoginState(); // Just to use 'self' and suppress warning, though technically refreshes login state which is harmless
           }
       }
       adw_dialog_close(ADW_DIALOG(d));
  }), this);

  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

void WorkshopView::setupDownloadCallbacks() {
  auto& dq = bwp::steam::DownloadQueue::getInstance();

  dq.setProgressCallback(
      [this](const std::string& /*workshopId*/, const bwp::steam::DownloadProgress& prog) {
        // Called from background thread — dispatch to main
        struct ProgData {
          WorkshopView* view;
          std::string title;
          float percent;
        };
        auto* pd = new ProgData{this, prog.title, prog.percent};
        g_idle_add(+[](gpointer data) -> gboolean {
          auto* d = static_cast<ProgData*>(data);
          d->view->showDownloadProgress(d->title, d->percent);
          delete d;
          return G_SOURCE_REMOVE;
        }, pd);
      });

  dq.setCompleteCallback(
      [this](const std::string& workshopId, bool success, const std::string& /*path*/) {
        // Look up the title from the queue before it's cleaned up
        std::string title;
        auto queue = bwp::steam::DownloadQueue::getInstance().getQueue();
        for (const auto& item : queue) {
          if (item.workshopId == workshopId) {
            title = item.title;
            break;
          }
        }
        if (title.empty()) title = workshopId;

        struct DoneData {
          WorkshopView* view;
          std::string title;
          bool success;
        };
        auto* dd = new DoneData{this, title, success};
        g_idle_add(+[](gpointer data) -> gboolean {
          auto* d = static_cast<DoneData*>(data);
          d->view->showDownloadComplete(d->title, d->success);
          delete d;
          return G_SOURCE_REMOVE;
        }, dd);
      });
}

void WorkshopView::showDownloadProgress(const std::string& title, float percent) {
  // Get the toast overlay from the app window
  auto *app = g_application_get_default();
  if (!app) return;
  auto *win = gtk_application_get_active_window(GTK_APPLICATION(app));
  if (!win) return;
  GtkWidget *content = adw_application_window_get_content(ADW_APPLICATION_WINDOW(win));
  if (!content || !ADW_IS_TOAST_OVERLAY(content)) return;

  // Format progress text
  int pct = static_cast<int>(percent);
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;
  std::string msg = "Downloading " + title + "... " + std::to_string(pct) + "%";

  if (m_downloadToast && m_downloadProgressLabel) {
    // Update existing toast label
    char *escaped = g_markup_escape_text(msg.c_str(), -1);
    std::string markup = "<span color='#93C5FD'>" + std::string(escaped) + "</span>";
    gtk_label_set_markup(GTK_LABEL(m_downloadProgressLabel), markup.c_str());
    g_free(escaped);
  } else {
    // Create new persistent toast
    dismissDownloadToast(); // clean up any stale one

    m_downloadingTitle = title;
    m_downloadToast = adw_toast_new(" ");
    adw_toast_set_timeout(m_downloadToast, 0); // persistent

    m_downloadProgressLabel = gtk_label_new(nullptr);
    char *escaped = g_markup_escape_text(msg.c_str(), -1);
    std::string markup = "<span color='#93C5FD'>" + std::string(escaped) + "</span>";
    gtk_label_set_markup(GTK_LABEL(m_downloadProgressLabel), markup.c_str());
    g_free(escaped);
    gtk_label_set_ellipsize(GTK_LABEL(m_downloadProgressLabel), PANGO_ELLIPSIZE_END);

    adw_toast_set_custom_title(m_downloadToast, m_downloadProgressLabel);

    // Track dismissal (user swiped away)
    g_signal_connect(m_downloadToast, "dismissed",
        G_CALLBACK(+[](AdwToast*, gpointer data) {
          auto* self = static_cast<WorkshopView*>(data);
          self->m_downloadToast = nullptr;
          self->m_downloadProgressLabel = nullptr;
        }), this);

    adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(content), m_downloadToast);
  }
}

void WorkshopView::showDownloadComplete(const std::string& title, bool success) {
  // Dismiss the progress toast
  dismissDownloadToast();

  // Show a timed completion toast
  bwp::core::utils::ToastRequest req;
  if (success) {
    req.message = "Downloaded " + title;
    req.type = bwp::core::utils::ToastType::Success;
  } else {
    req.message = "Failed to download " + title;
    req.type = bwp::core::utils::ToastType::Error;
  }
  req.durationMs = 5000;
  bwp::core::utils::ToastManager::getInstance().showToast(req);
}

void WorkshopView::dismissDownloadToast() {
  if (m_downloadToast) {
    adw_toast_dismiss(m_downloadToast);
    m_downloadToast = nullptr;
    m_downloadProgressLabel = nullptr;
  }
}

}
