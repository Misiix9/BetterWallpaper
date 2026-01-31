#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <vector>

namespace bwp::gui {

class PreviewPanel {
public:
  PreviewPanel();
  ~PreviewPanel();

  GtkWidget *getWidget() const { return m_box; }

  void setWallpaper(const bwp::wallpaper::WallpaperInfo &info);
  void clear();

private:
  void setupUi();
  void onApplyClicked();
  void updateMonitorList();
  void loadThumbnail(const std::string &path);
  bool setWallpaperWithTool(const std::string &path,
                            const std::string &monitor);
  std::string getSettingsFlags();

  GtkWidget *m_box;
  GtkWidget *m_imageStack;
  GtkWidget *m_picture1;
  GtkWidget *m_picture2;
  GtkWidget *m_titleLabel;
  GtkWidget *m_detailsLabel;
  GtkWidget *m_applyButton;
  GtkWidget *m_monitorDropdown;
  GtkWidget *m_statusLabel;
  GtkWidget *m_ratingBox;
  std::vector<GtkWidget *> m_ratingButtons;

  // Helpers
  void setupRating();
  void updateRatingDisplay();
  void setRating(int rating);

  // Settings Controls
  GtkWidget *m_silentCheck;
  GtkWidget *m_noAudioProcCheck;
  GtkWidget *m_disableMouseCheck;
  GtkWidget *m_noAutomuteCheck;
  GtkWidget *m_fpsSpin;
  GtkWidget *m_volumeScale;
  GtkWidget *m_scalingDropdown;

  std::vector<std::string> m_monitorNames;
  bwp::wallpaper::WallpaperInfo m_currentInfo;
};

} // namespace bwp::gui
