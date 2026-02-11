#pragma once
#include <gtk/gtk.h>
#include <memory>
#include <string>
namespace bwp::gui {
class MiniPlayerWindow {
public:
  MiniPlayerWindow();
  ~MiniPlayerWindow();
  void show();
  void hide();
  void toggle();
  bool isVisible() const;
  void setTitle(const std::string &title);
  void setThumbnail(const std::string &path);
  void setPlaying(bool playing);
  void setVolume(double volume);
private:
  void setupUi();
  GtkWidget *m_window;
  GtkWidget *m_mainBox;
  GtkWidget *m_image;
  GtkWidget *m_titleLabel;
  GtkWidget *m_playPauseBtn;
  GtkWidget *m_prevBtn;
  GtkWidget *m_nextBtn;
  GtkWidget *m_volumeScale;
  GtkWidget *m_settingsBtn;
  bool m_isPlaying = false;
};
}  
