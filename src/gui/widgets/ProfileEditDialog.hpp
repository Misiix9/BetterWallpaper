#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
namespace bwp::gui {
class ProfileEditDialog {
public:
  using SaveCallback = std::function<void(const nlohmann::json &profile)>;
  explicit ProfileEditDialog(GtkWindow *parent);
  ~ProfileEditDialog();
  void show();
  void show(const nlohmann::json &profile);
  void setSaveCallback(SaveCallback callback) { m_saveCallback = callback; }
private:
  void setupUi();
  void createBasicInfoSection();
  void createMonitorSection();
  void populateFromProfile(const nlohmann::json &profile);
  nlohmann::json buildProfile() const;
  void onSave();
  GtkWidget *m_dialog = nullptr;
  GtkWidget *m_nameEntry = nullptr;
  GtkWidget *m_descEntry = nullptr;
  GtkWidget *m_monitorGroup = nullptr;
  std::string m_profileId;
  nlohmann::json m_currentProfile;
  SaveCallback m_saveCallback;
};
}  
