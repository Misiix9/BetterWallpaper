#pragma once
#include "../../core/transition/Easing.hpp"
#include "../../core/transition/TransitionEffect.hpp"
#include <adwaita.h>
#include <functional>
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <vector>
namespace bwp::gui {
class TransitionDialog {
public:
  struct TransitionSettings {
    std::string effectName = "Fade";
    std::string easingName = "easeInOut";
    int durationMs = 500;
  };
  using ApplyCallback = std::function<void(const TransitionSettings &settings)>;
  explicit TransitionDialog(GtkWindow *parent);
  ~TransitionDialog();
  void show();
  void show(const TransitionSettings &currentSettings);
  void presentTo(GtkWidget *parent);
  void setCallback(ApplyCallback callback) { m_callback = callback; }
  void setPreviewImages(const std::string &fromPath, const std::string &toPath);
private:
  void setupUi();
  void createEffectSelector();
  void createEasingSelector();
  void createDurationSlider();
  void createPreviewArea();
  void updatePreview();
  void runPreviewTransition();
  void onApply();
  TransitionSettings getCurrentSettings() const;
  std::shared_ptr<bwp::transition::TransitionEffect>
  createEffect(const std::string &name) const;
  static void onDrawEasingPreview(GtkDrawingArea *area, cairo_t *cr, int width,
                                  int height, gpointer data);
  static void onDrawTransitionPreview(GtkDrawingArea *area, cairo_t *cr,
                                      int width, int height, gpointer data);
  GtkWidget *m_dialog = nullptr;
  GtkWidget *m_effectDropdown = nullptr;
  GtkWidget *m_easingDropdown = nullptr;
  GtkWidget *m_durationScale = nullptr;
  GtkWidget *m_durationLabel = nullptr;
  GtkWidget *m_previewArea = nullptr;
  GtkWidget *m_previewButton = nullptr;
  cairo_surface_t *m_previewFrom = nullptr;
  cairo_surface_t *m_previewTo = nullptr;
  bool m_previewRunning = false;
  double m_previewProgress = 0.0;
  guint m_previewTimerId = 0;
  GtkWidget *m_easingPreview = nullptr;
  ApplyCallback m_callback;
  std::vector<std::string> m_effectNames;
  std::vector<std::string> m_easingNames;
};
}  
