#pragma once
#include "../../core/wallpaper/WallpaperInfo.hpp"
#include <gtk/gtk.h>
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
  void setupRating();
  void updateRatingDisplay();
  void setRating(int rating);
  void saveCurrentSettings();
  GtkWidget *m_silentCheck;
  GtkWidget *m_noAudioProcCheck;
  GtkWidget *m_disableMouseCheck;
  GtkWidget *m_noAutomuteCheck;
  GtkWidget *m_fpsSpin;
  GtkWidget *m_volumeScale;
  GtkWidget *m_scalingDropdown;
  std::vector<std::string> m_monitorNames;
  bwp::wallpaper::WallpaperInfo m_currentInfo;
  bool m_updatingWidgets = false;
  GtkWidget *m_scrolledWindow;
  GtkWidget *m_zoomIndicator = nullptr;
  double m_zoomLevel = 1.0;
  double m_dragStartX = 0;
  double m_dragStartY = 0;
  static constexpr double ZOOM_MIN = 1.0;
  static constexpr double ZOOM_MAX = 5.0;
  static constexpr double ZOOM_STEP = 0.25;
  void setupGestures(GtkWidget *widget);
  void applyZoom(double newZoom, double focusX = -1, double focusY = -1);
  void resetZoom();
  void updateZoomIndicator();
  void onPanGesture(GtkGestureDrag *gesture, double offset_x, double offset_y);
  void onPanBegin(GtkGestureDrag *gesture, double start_x, double start_y);

  guint m_preloadSourceId = 0;
  std::string m_lastPreloadPath;
};
} // namespace bwp::gui
