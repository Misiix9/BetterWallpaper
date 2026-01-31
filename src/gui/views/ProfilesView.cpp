#include "ProfilesView.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/utils/Logger.hpp"
#include <filesystem>

namespace bwp::gui {

ProfilesView::ProfilesView() {
  setupUi();
  loadProfiles();
}

ProfilesView::~ProfilesView() {}

void ProfilesView::setupUi() {
  // Main scrollable container
  m_content = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_content),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_start(box, 24);
  gtk_widget_set_margin_end(box, 24);
  gtk_widget_set_margin_top(box, 24);
  gtk_widget_set_margin_bottom(box, 24);

  // Header with title and create button
  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_bottom(header, 24);

  GtkWidget *title = gtk_label_new("Wallpaper Profiles");
  gtk_widget_add_css_class(title, "title-1");
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_widget_set_hexpand(title, TRUE);
  gtk_box_append(GTK_BOX(header), title);

  GtkWidget *createBtn = gtk_button_new_with_label("New Profile");
  gtk_button_set_icon_name(GTK_BUTTON(createBtn), "list-add-symbolic");
  gtk_widget_add_css_class(createBtn, "suggested-action");
  g_signal_connect(createBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *, gpointer data) {
                     ProfilesView *self = static_cast<ProfilesView *>(data);
                     self->onCreateProfile();
                   }),
                   this);
  gtk_box_append(GTK_BOX(header), createBtn);

  gtk_box_append(GTK_BOX(box), header);

  // Description
  GtkWidget *desc = gtk_label_new(
      "Create and manage wallpaper configurations for different activities or "
      "setups. Profiles store wallpaper assignments, settings, and can be "
      "triggered by schedules.");
  gtk_widget_add_css_class(desc, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
  gtk_label_set_xalign(GTK_LABEL(desc), 0);
  gtk_widget_set_margin_bottom(desc, 24);
  gtk_box_append(GTK_BOX(box), desc);

  // Empty state (shown when no profiles exist)
  m_emptyState = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_valign(m_emptyState, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(m_emptyState, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(m_emptyState, 48);
  gtk_widget_set_margin_bottom(m_emptyState, 48);

  GtkWidget *emptyIcon =
      gtk_image_new_from_icon_name("document-properties-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(emptyIcon), 64);
  gtk_widget_add_css_class(emptyIcon, "dim-label");
  gtk_box_append(GTK_BOX(m_emptyState), emptyIcon);

  GtkWidget *emptyTitle = gtk_label_new("No Profiles Yet");
  gtk_widget_add_css_class(emptyTitle, "title-2");
  gtk_box_append(GTK_BOX(m_emptyState), emptyTitle);

  GtkWidget *emptyDesc = gtk_label_new(
      "Create your first profile to save wallpaper configurations");
  gtk_widget_add_css_class(emptyDesc, "dim-label");
  gtk_box_append(GTK_BOX(m_emptyState), emptyDesc);

  gtk_box_append(GTK_BOX(box), m_emptyState);

  // Profile cards grid
  m_flowBox = gtk_flow_box_new();
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(m_flowBox), GTK_SELECTION_NONE);
  gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(m_flowBox), TRUE);
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(m_flowBox), 1);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(m_flowBox), 3);
  gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(m_flowBox), 16);
  gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(m_flowBox), 16);
  gtk_widget_set_visible(m_flowBox, FALSE); // Hidden until profiles exist

  gtk_box_append(GTK_BOX(box), m_flowBox);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_content), box);
}

void ProfilesView::loadProfiles() {
  // Clear existing cards
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(m_flowBox)) != nullptr) {
    gtk_flow_box_remove(GTK_FLOW_BOX(m_flowBox), child);
  }

  // Load profiles from config
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");
  m_activeProfileId = conf.get<std::string>("profiles.active");

  if (profiles.empty()) {
    // Show empty state
    gtk_widget_set_visible(m_emptyState, TRUE);
    gtk_widget_set_visible(m_flowBox, FALSE);
  } else {
    gtk_widget_set_visible(m_emptyState, FALSE);
    gtk_widget_set_visible(m_flowBox, TRUE);

    for (const auto &profile : profiles) {
      std::string id = profile.value("id", "");
      std::string name = profile.value("name", "Unnamed Profile");
      std::string desc = profile.value("description", "");
      bool isActive = (id == m_activeProfileId);

      GtkWidget *card = createProfileCard(id, name, desc, isActive);
      gtk_flow_box_append(GTK_FLOW_BOX(m_flowBox), card);
    }
  }
}

void ProfilesView::refresh() { loadProfiles(); }

GtkWidget *ProfilesView::createProfileCard(const std::string &id,
                                           const std::string &name,
                                           const std::string &description,
                                           bool isActive) {
  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_add_css_class(card, "card");
  gtk_widget_set_size_request(card, 280, -1);
  gtk_widget_set_margin_start(card, 8);
  gtk_widget_set_margin_end(card, 8);
  gtk_widget_set_margin_top(card, 8);
  gtk_widget_set_margin_bottom(card, 8);

  // Header with icon and active badge
  GtkWidget *headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(headerBox, 12);
  gtk_widget_set_margin_end(headerBox, 12);
  gtk_widget_set_margin_top(headerBox, 12);

  GtkWidget *icon =
      gtk_image_new_from_icon_name("document-properties-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 32);
  gtk_box_append(GTK_BOX(headerBox), icon);

  GtkWidget *titleLabel = gtk_label_new(name.c_str());
  gtk_widget_add_css_class(titleLabel, "title-4");
  gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(titleLabel, TRUE);
  gtk_label_set_ellipsize(GTK_LABEL(titleLabel), PANGO_ELLIPSIZE_END);
  gtk_box_append(GTK_BOX(headerBox), titleLabel);

  if (isActive) {
    GtkWidget *badge = gtk_label_new("Active");
    gtk_widget_add_css_class(badge, "success");
    gtk_widget_add_css_class(badge, "caption");
    gtk_box_append(GTK_BOX(headerBox), badge);
  }

  gtk_box_append(GTK_BOX(card), headerBox);

  // Description
  if (!description.empty()) {
    GtkWidget *descLabel = gtk_label_new(description.c_str());
    gtk_widget_add_css_class(descLabel, "dim-label");
    gtk_widget_set_halign(descLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_start(descLabel, 12);
    gtk_widget_set_margin_end(descLabel, 12);
    gtk_label_set_wrap(GTK_LABEL(descLabel), TRUE);
    gtk_label_set_lines(GTK_LABEL(descLabel), 2);
    gtk_label_set_ellipsize(GTK_LABEL(descLabel), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(card), descLabel);
  }

  // Action buttons
  GtkWidget *actionsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(actionsBox, GTK_ALIGN_END);
  gtk_widget_set_margin_start(actionsBox, 12);
  gtk_widget_set_margin_end(actionsBox, 12);
  gtk_widget_set_margin_top(actionsBox, 8);
  gtk_widget_set_margin_bottom(actionsBox, 12);

  // Store profile ID for callbacks
  std::string *idCopy = new std::string(id);

  // Activate button (only if not already active)
  if (!isActive) {
    GtkWidget *activateBtn = gtk_button_new_with_label("Activate");
    gtk_widget_add_css_class(activateBtn, "suggested-action");
    g_object_set_data_full(
        G_OBJECT(activateBtn), "profile-id", idCopy,
        [](gpointer data) { delete static_cast<std::string *>(data); });
    g_signal_connect(activateBtn, "clicked",
                     G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       ProfilesView *self = static_cast<ProfilesView *>(data);
                       std::string *pid = static_cast<std::string *>(
                           g_object_get_data(G_OBJECT(btn), "profile-id"));
                       if (pid)
                         self->onActivateProfile(*pid);
                     }),
                     this);
    gtk_box_append(GTK_BOX(actionsBox), activateBtn);
  }

  // Edit button
  GtkWidget *editBtn = gtk_button_new_from_icon_name("document-edit-symbolic");
  gtk_widget_add_css_class(editBtn, "flat");
  std::string *idCopy2 = new std::string(id);
  g_object_set_data_full(
      G_OBJECT(editBtn), "profile-id", idCopy2,
      [](gpointer data) { delete static_cast<std::string *>(data); });
  g_signal_connect(editBtn, "clicked",
                   G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                     ProfilesView *self = static_cast<ProfilesView *>(data);
                     std::string *pid = static_cast<std::string *>(
                         g_object_get_data(G_OBJECT(btn), "profile-id"));
                     if (pid)
                       self->onEditProfile(*pid);
                   }),
                   this);
  gtk_box_append(GTK_BOX(actionsBox), editBtn);

  // More actions menu button
  GtkWidget *moreBtn = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(moreBtn), "view-more-symbolic");
  gtk_widget_add_css_class(moreBtn, "flat");

  // Create popover menu
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Duplicate", "profile.duplicate");
  g_menu_append(menu, "Export", "profile.export");
  g_menu_append(menu, "Delete", "profile.delete");

  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(moreBtn), popover);
  g_object_unref(menu);

  gtk_box_append(GTK_BOX(actionsBox), moreBtn);

  gtk_box_append(GTK_BOX(card), actionsBox);

  return card;
}

void ProfilesView::onCreateProfile() {
  LOG_INFO("Create new profile");

  // Generate a unique ID
  std::string newId =
      "profile_" +
      std::to_string(
          std::chrono::system_clock::now().time_since_epoch().count());

  auto &conf = bwp::config::ConfigManager::getInstance();
  auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");

  nlohmann::json newProfile;
  newProfile["id"] = newId;
  newProfile["name"] = "New Profile";
  newProfile["description"] = "";
  newProfile["monitors"] = nlohmann::json::object();
  newProfile["triggers"] = nlohmann::json::array();

  profiles.push_back(newProfile);
  conf.set("profiles.list", profiles);

  loadProfiles();

  // TODO: Open edit dialog for the new profile
  onEditProfile(newId);
}

void ProfilesView::onActivateProfile(const std::string &profileId) {
  LOG_INFO("Activating profile: " + profileId);

  auto &conf = bwp::config::ConfigManager::getInstance();
  conf.set("profiles.active", profileId);
  m_activeProfileId = profileId;

  // TODO: Apply the profile's wallpaper configuration

  loadProfiles();
}

void ProfilesView::onEditProfile(const std::string &profileId) {
  LOG_INFO("Edit profile: " + profileId);

  // Find the profile data
  auto &conf = bwp::config::ConfigManager::getInstance();
  auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");

  nlohmann::json profileToEdit;
  for (const auto &profile : profiles) {
    if (profile.value("id", "") == profileId) {
      profileToEdit = profile;
      break;
    }
  }

  if (profileToEdit.empty()) {
    LOG_ERROR("Profile not found: " + profileId);
    return;
  }

  // Get parent window
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(m_content));

  // Create dialog if needed
  if (!m_editDialog) {
    m_editDialog = std::make_unique<ProfileEditDialog>(parent);
    m_editDialog->setSaveCallback([this](const nlohmann::json &profile) {
      // Save the profile back to config
      auto &conf = bwp::config::ConfigManager::getInstance();
      auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");

      std::string profileId = profile.value("id", "");
      bool found = false;

      // Update existing profile
      for (auto &p : profiles) {
        if (p.value("id", "") == profileId) {
          p = profile;
          found = true;
          break;
        }
      }

      // If not found (new profile), add it
      if (!found) {
        profiles.push_back(profile);
      }

      conf.set("profiles.list", profiles);
      loadProfiles();
      LOG_INFO("Profile saved: " + profile.value("name", ""));
    });
  }

  m_editDialog->show(profileToEdit);
}

void ProfilesView::onDeleteProfile(const std::string &profileId) {
  LOG_INFO("Delete profile: " + profileId);

  auto &conf = bwp::config::ConfigManager::getInstance();
  auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");

  // Remove the profile
  profiles.erase(std::remove_if(profiles.begin(), profiles.end(),
                                [&profileId](const nlohmann::json &p) {
                                  return p.value("id", "") == profileId;
                                }),
                 profiles.end());

  conf.set("profiles.list", profiles);

  // Clear active if it was the deleted one
  if (m_activeProfileId == profileId) {
    conf.set("profiles.active", "");
    m_activeProfileId = "";
  }

  loadProfiles();
}

void ProfilesView::onDuplicateProfile(const std::string &profileId) {
  LOG_INFO("Duplicate profile: " + profileId);

  auto &conf = bwp::config::ConfigManager::getInstance();
  auto profiles = conf.get<std::vector<nlohmann::json>>("profiles.list");

  // Find the profile to duplicate
  for (const auto &profile : profiles) {
    if (profile.value("id", "") == profileId) {
      nlohmann::json copy = profile;
      copy["id"] =
          "profile_" +
          std::to_string(
              std::chrono::system_clock::now().time_since_epoch().count());
      copy["name"] = profile.value("name", "Profile") + " (Copy)";
      profiles.push_back(copy);
      break;
    }
  }

  conf.set("profiles.list", profiles);
  loadProfiles();
}

} // namespace bwp::gui
