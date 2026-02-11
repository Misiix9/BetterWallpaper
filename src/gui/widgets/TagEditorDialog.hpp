#pragma once
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>
namespace bwp::gui {
class TagEditorDialog {
public:
  explicit TagEditorDialog(GtkWindow *parent);
  ~TagEditorDialog();
  void show(const std::string &wallpaperId,
            const std::vector<std::string> &currentTags);
  using ApplyCallback = std::function<void(
      const std::string &id, const std::vector<std::string> &newTags)>;
  void setCallback(ApplyCallback callback);
private:
  void setupUi();
  void updateTagList();
  void addTag(const std::string &tag);
  void removeTag(const std::string &tag);
  GtkWidget *m_dialog;
  GtkWidget *m_entry;
  GtkWidget *m_tagBox;  
  std::string m_currentId;
  std::vector<std::string> m_currentTags;
  ApplyCallback m_callback;
};
}  
