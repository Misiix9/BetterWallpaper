#pragma once
#include "../widgets/ProfileEditDialog.hpp"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <vector>

namespace bwp::gui {

class ProfilesView {
public:
  ProfilesView();
  ~ProfilesView();

  GtkWidget *getWidget() const { return m_content; }
  void refresh();

private:
  void setupUi();
  void loadProfiles();
  void onCreateProfile();
  void onActivateProfile(const std::string &profileId);
  void onEditProfile(const std::string &profileId);
  void onDeleteProfile(const std::string &profileId);
  void onDuplicateProfile(const std::string &profileId);

  GtkWidget *createProfileCard(const std::string &id, const std::string &name,
                               const std::string &description, bool isActive);

  GtkWidget *m_content = nullptr;
  GtkWidget *m_flowBox = nullptr;
  GtkWidget *m_emptyState = nullptr;

  std::string m_activeProfileId;
  std::unique_ptr<ProfileEditDialog> m_editDialog;
};

} // namespace bwp::gui
