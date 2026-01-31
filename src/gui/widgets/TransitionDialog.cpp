#include "TransitionDialog.hpp"
#include "../../core/transition/effects/AdvancedEffects.hpp"
#include "../../core/transition/effects/BasicEffects.hpp"
#include "../../core/utils/Logger.hpp"
#include <cmath>

namespace bwp::gui {

TransitionDialog::TransitionDialog(GtkWindow *parent) {
  // Initialize effect names
  m_effectNames = {"Fade",
                   "Slide",
                   "Wipe",
                   "Expanding Circle",
                   "Expanding Square",
                   "Dissolve",
                   "Zoom",
                   "Morph",
                   "Angled Wipe",
                   "Pixelate",
                   "Blinds"};

  // Initialize easing names
  m_easingNames = bwp::transition::Easing::getAvailableNames();

  // Create dialog
  m_dialog = GTK_WIDGET(adw_dialog_new());
  adw_dialog_set_title(ADW_DIALOG(m_dialog), "Transition Settings");
  adw_dialog_set_content_width(ADW_DIALOG(m_dialog), 450);
  adw_dialog_set_content_height(ADW_DIALOG(m_dialog), 550);

  (void)parent; // May be used later for positioning

  setupUi();
}

TransitionDialog::~TransitionDialog() {
  if (m_previewTimerId > 0) {
    g_source_remove(m_previewTimerId);
  }
  if (m_previewFrom) {
    cairo_surface_destroy(m_previewFrom);
  }
  if (m_previewTo) {
    cairo_surface_destroy(m_previewTo);
  }
}

void TransitionDialog::setupUi() {
  // Main container
  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  // Header bar
  GtkWidget *headerBar = adw_header_bar_new();
  adw_header_bar_set_show_start_title_buttons(ADW_HEADER_BAR(headerBar), FALSE);
  adw_header_bar_set_show_end_title_buttons(ADW_HEADER_BAR(headerBar), FALSE);

  // Cancel button
  GtkWidget *cancelBtn = gtk_button_new_with_label("Cancel");
  g_signal_connect(
      cancelBtn, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<TransitionDialog *>(data);
        adw_dialog_close(ADW_DIALOG(self->m_dialog));
      }),
      this);
  adw_header_bar_pack_start(ADW_HEADER_BAR(headerBar), cancelBtn);

  // Apply button
  GtkWidget *applyBtn = gtk_button_new_with_label("Apply");
  gtk_widget_add_css_class(applyBtn, "suggested-action");
  g_signal_connect(
      applyBtn, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<TransitionDialog *>(data);
        self->onApply();
        adw_dialog_close(ADW_DIALOG(self->m_dialog));
      }),
      this);
  adw_header_bar_pack_end(ADW_HEADER_BAR(headerBar), applyBtn);

  gtk_box_append(GTK_BOX(content), headerBar);

  // Scrollable content
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scrolled, TRUE);

  GtkWidget *innerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_margin_start(innerBox, 16);
  gtk_widget_set_margin_end(innerBox, 16);
  gtk_widget_set_margin_top(innerBox, 16);
  gtk_widget_set_margin_bottom(innerBox, 16);

  // Preview area
  createPreviewArea();
  gtk_box_append(GTK_BOX(innerBox), m_previewArea);

  // Settings group
  GtkWidget *settingsGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(settingsGroup),
                                  "Transition Settings");

  // Effect selector
  createEffectSelector();
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(settingsGroup),
                            m_effectDropdown);

  // Duration slider
  createDurationSlider();
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(settingsGroup),
                            m_durationScale);

  // Easing selector
  createEasingSelector();
  adw_preferences_group_add(ADW_PREFERENCES_GROUP(settingsGroup),
                            m_easingDropdown);

  gtk_box_append(GTK_BOX(innerBox), settingsGroup);

  // Easing curve preview
  GtkWidget *easingGroup = adw_preferences_group_new();
  adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(easingGroup),
                                  "Easing Curve");

  m_easingPreview = gtk_drawing_area_new();
  gtk_widget_set_size_request(m_easingPreview, -1, 100);
  gtk_widget_add_css_class(m_easingPreview, "card");

  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(m_easingPreview),
                                 onDrawEasingPreview, this, nullptr);

  adw_preferences_group_add(ADW_PREFERENCES_GROUP(easingGroup), m_easingPreview);
  gtk_box_append(GTK_BOX(innerBox), easingGroup);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), innerBox);
  gtk_box_append(GTK_BOX(content), scrolled);

  adw_dialog_set_child(ADW_DIALOG(m_dialog), content);
}

void TransitionDialog::createEffectSelector() {
  // Create string list for dropdown
  GtkStringList *effectList = gtk_string_list_new(nullptr);
  for (const auto &name : m_effectNames) {
    gtk_string_list_append(effectList, name.c_str());
  }

  m_effectDropdown = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_effectDropdown), "Effect");
  adw_combo_row_set_model(ADW_COMBO_ROW(m_effectDropdown),
                          G_LIST_MODEL(effectList));

  g_signal_connect(
      m_effectDropdown, "notify::selected",
      G_CALLBACK(+[](AdwComboRow *, GParamSpec *, gpointer data) {
        auto *self = static_cast<TransitionDialog *>(data);
        self->updatePreview();
      }),
      this);
}

void TransitionDialog::createEasingSelector() {
  GtkStringList *easingList = gtk_string_list_new(nullptr);
  for (const auto &name : m_easingNames) {
    gtk_string_list_append(easingList, name.c_str());
  }

  m_easingDropdown = adw_combo_row_new();
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_easingDropdown), "Easing");
  adw_combo_row_set_model(ADW_COMBO_ROW(m_easingDropdown),
                          G_LIST_MODEL(easingList));

  // Default to easeInOut (index 3)
  adw_combo_row_set_selected(ADW_COMBO_ROW(m_easingDropdown), 3);

  g_signal_connect(
      m_easingDropdown, "notify::selected",
      G_CALLBACK(+[](AdwComboRow *, GParamSpec *, gpointer data) {
        auto *self = static_cast<TransitionDialog *>(data);
        gtk_widget_queue_draw(self->m_easingPreview);
        self->updatePreview();
      }),
      this);
}

void TransitionDialog::createDurationSlider() {
  m_durationScale = adw_spin_row_new_with_range(100, 3000, 50);
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_durationScale),
                                "Duration (ms)");
  adw_spin_row_set_value(ADW_SPIN_ROW(m_durationScale), 500);

  g_signal_connect(
      m_durationScale, "notify::value",
      G_CALLBACK(+[](AdwSpinRow *, GParamSpec *, gpointer data) {
        auto *self = static_cast<TransitionDialog *>(data);
        self->updatePreview();
      }),
      this);
}

void TransitionDialog::createPreviewArea() {
  GtkWidget *previewBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

  // Preview drawing area
  GtkWidget *previewFrame = gtk_frame_new(nullptr);
  gtk_widget_add_css_class(previewFrame, "card");

  GtkWidget *previewDrawing = gtk_drawing_area_new();
  gtk_widget_set_size_request(previewDrawing, -1, 150);

  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(previewDrawing),
                                 onDrawTransitionPreview, this, nullptr);
  // Store drawing area reference for later
  g_object_set_data(G_OBJECT(m_previewArea), "draw-area", previewDrawing);

  gtk_frame_set_child(GTK_FRAME(previewFrame), previewDrawing);
  gtk_box_append(GTK_BOX(previewBox), previewFrame);

  // Preview button
  m_previewButton = gtk_button_new_with_label("Preview Transition");
  gtk_widget_add_css_class(m_previewButton, "pill");
  gtk_widget_set_halign(m_previewButton, GTK_ALIGN_CENTER);

  g_signal_connect(
      m_previewButton, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<TransitionDialog *>(data);
        self->runPreviewTransition();
      }),
      this);

  gtk_box_append(GTK_BOX(previewBox), m_previewButton);

  m_previewArea = previewBox;
}

void TransitionDialog::updatePreview() {
  // Could update static preview here if needed
}

void TransitionDialog::runPreviewTransition() {
  if (m_previewRunning) {
    return;
  }

  // Create sample surfaces if not set
  if (!m_previewFrom || !m_previewTo) {
    // Create placeholder preview surfaces
    int previewWidth = 400;
    int previewHeight = 150;

    if (m_previewFrom) {
      cairo_surface_destroy(m_previewFrom);
    }
    if (m_previewTo) {
      cairo_surface_destroy(m_previewTo);
    }

    m_previewFrom = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, previewWidth,
                                               previewHeight);
    cairo_t *crFrom = cairo_create(m_previewFrom);
    // Gradient blue
    cairo_pattern_t *patternFrom =
        cairo_pattern_create_linear(0, 0, previewWidth, previewHeight);
    cairo_pattern_add_color_stop_rgb(patternFrom, 0, 0.2, 0.3, 0.6);
    cairo_pattern_add_color_stop_rgb(patternFrom, 1, 0.1, 0.2, 0.4);
    cairo_set_source(crFrom, patternFrom);
    cairo_paint(crFrom);
    cairo_pattern_destroy(patternFrom);
    cairo_destroy(crFrom);

    m_previewTo = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, previewWidth,
                                             previewHeight);
    cairo_t *crTo = cairo_create(m_previewTo);
    // Gradient orange
    cairo_pattern_t *patternTo =
        cairo_pattern_create_linear(0, 0, previewWidth, previewHeight);
    cairo_pattern_add_color_stop_rgb(patternTo, 0, 0.6, 0.4, 0.2);
    cairo_pattern_add_color_stop_rgb(patternTo, 1, 0.4, 0.2, 0.1);
    cairo_set_source(crTo, patternTo);
    cairo_paint(crTo);
    cairo_pattern_destroy(patternTo);
    cairo_destroy(crTo);
  }

  m_previewRunning = true;
  m_previewProgress = 0.0;

  int frameInterval = 16; // ~60fps

  // Start animation timer
  m_previewTimerId = g_timeout_add(
      frameInterval,
      [](gpointer data) -> gboolean {
        auto *self = static_cast<TransitionDialog *>(data);

        self->m_previewProgress += 16.0 / self->getCurrentSettings().durationMs;

        // Get stored draw area
        GtkWidget *drawArea = static_cast<GtkWidget *>(
            g_object_get_data(G_OBJECT(self->m_previewArea), "draw-area"));

        if (self->m_previewProgress >= 1.0) {
          self->m_previewProgress = 1.0;
          self->m_previewRunning = false;
          self->m_previewTimerId = 0;

          if (drawArea) {
            gtk_widget_queue_draw(drawArea);
          }

          return G_SOURCE_REMOVE;
        }

        if (drawArea) {
          gtk_widget_queue_draw(drawArea);
        }

        return G_SOURCE_CONTINUE;
      },
      this);
}

void TransitionDialog::setPreviewImages(const std::string &fromPath,
                                        const std::string &toPath) {
  // Load images from paths
  // For now, this is a placeholder - would load actual images
  LOG_DEBUG("Setting preview images: " + fromPath + " -> " + toPath);
}

TransitionDialog::TransitionSettings
TransitionDialog::getCurrentSettings() const {
  TransitionSettings settings;

  guint effectIdx = adw_combo_row_get_selected(ADW_COMBO_ROW(m_effectDropdown));
  if (effectIdx < m_effectNames.size()) {
    settings.effectName = m_effectNames[effectIdx];
  }

  guint easingIdx = adw_combo_row_get_selected(ADW_COMBO_ROW(m_easingDropdown));
  if (easingIdx < m_easingNames.size()) {
    settings.easingName = m_easingNames[easingIdx];
  }

  settings.durationMs =
      static_cast<int>(adw_spin_row_get_value(ADW_SPIN_ROW(m_durationScale)));

  return settings;
}

std::shared_ptr<bwp::transition::TransitionEffect>
TransitionDialog::createEffect(const std::string &name) const {
  if (name == "Fade") {
    return std::make_shared<bwp::transition::FadeEffect>();
  } else if (name == "Slide") {
    return std::make_shared<bwp::transition::SlideEffect>();
  } else if (name == "Wipe") {
    return std::make_shared<bwp::transition::WipeEffect>();
  } else if (name == "Expanding Circle") {
    return std::make_shared<bwp::transition::ExpandingCircleEffect>();
  } else if (name == "Expanding Square") {
    return std::make_shared<bwp::transition::ExpandingSquareEffect>();
  } else if (name == "Dissolve") {
    return std::make_shared<bwp::transition::DissolveEffect>();
  } else if (name == "Zoom") {
    return std::make_shared<bwp::transition::ZoomEffect>();
  } else if (name == "Morph") {
    return std::make_shared<bwp::transition::MorphEffect>();
  } else if (name == "Angled Wipe") {
    auto effect = std::make_shared<bwp::transition::AngledWipeEffect>();
    effect->setAngle(45);
    effect->setSoftEdge(true);
    return effect;
  } else if (name == "Pixelate") {
    return std::make_shared<bwp::transition::PixelateEffect>();
  } else if (name == "Blinds") {
    return std::make_shared<bwp::transition::BlindsEffect>();
  }

  // Default to fade
  return std::make_shared<bwp::transition::FadeEffect>();
}

void TransitionDialog::onApply() {
  if (m_callback) {
    m_callback(getCurrentSettings());
  }
}

void TransitionDialog::show() {
  TransitionSettings defaults;
  show(defaults);
}

void TransitionDialog::show(const TransitionSettings &currentSettings) {
  // Set UI to match current settings
  for (size_t idx = 0; idx < m_effectNames.size(); ++idx) {
    if (m_effectNames[idx] == currentSettings.effectName) {
      adw_combo_row_set_selected(ADW_COMBO_ROW(m_effectDropdown), idx);
      break;
    }
  }

  for (size_t idx = 0; idx < m_easingNames.size(); ++idx) {
    if (m_easingNames[idx] == currentSettings.easingName) {
      adw_combo_row_set_selected(ADW_COMBO_ROW(m_easingDropdown), idx);
      break;
    }
  }

  adw_spin_row_set_value(ADW_SPIN_ROW(m_durationScale),
                         currentSettings.durationMs);

  // Present the dialog
  // Find the parent window
  GtkWidget *parent = gtk_widget_get_ancestor(m_dialog, GTK_TYPE_WINDOW);
  if (parent) {
    adw_dialog_present(ADW_DIALOG(m_dialog), parent);
  }
}

// Static drawing callbacks
void TransitionDialog::onDrawEasingPreview(GtkDrawingArea * /*area*/,
                                           cairo_t *cr, int width, int height,
                                           gpointer data) {
  auto *self = static_cast<TransitionDialog *>(data);

  // Background
  cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 1.0);
  cairo_paint(cr);

  // Get current easing function
  auto settings = self->getCurrentSettings();
  auto easingFunc = bwp::transition::Easing::getByName(settings.easingName);

  // Draw grid
  cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
  cairo_set_line_width(cr, 1);
  for (int verticalLine = 0; verticalLine <= 4; ++verticalLine) {
    double xPos = verticalLine * width / 4.0;
    cairo_move_to(cr, xPos, 0);
    cairo_line_to(cr, xPos, height);
  }
  for (int horizontalLine = 0; horizontalLine <= 4; ++horizontalLine) {
    double yPos = horizontalLine * height / 4.0;
    cairo_move_to(cr, 0, yPos);
    cairo_line_to(cr, width, yPos);
  }
  cairo_stroke(cr);

  // Draw easing curve
  cairo_set_source_rgba(cr, 0.4, 0.6, 1.0, 1.0);
  cairo_set_line_width(cr, 2);

  double padding = 10;
  double drawWidth = width - 2 * padding;
  double drawHeight = height - 2 * padding;

  bool first = true;
  for (int step = 0; step <= 100; ++step) {
    double progress = step / 100.0;
    double eased = easingFunc(progress);

    double xPos = padding + progress * drawWidth;
    double yPos = padding + (1.0 - eased) * drawHeight;

    if (first) {
      cairo_move_to(cr, xPos, yPos);
      first = false;
    } else {
      cairo_line_to(cr, xPos, yPos);
    }
  }
  cairo_stroke(cr);
}

void TransitionDialog::onDrawTransitionPreview(GtkDrawingArea * /*area*/,
                                               cairo_t *cr, int width,
                                               int height, gpointer data) {
  auto *self = static_cast<TransitionDialog *>(data);

  // Draw background
  cairo_set_source_rgba(cr, 0.15, 0.15, 0.15, 1.0);
  cairo_paint(cr);

  if (self->m_previewRunning && self->m_previewFrom && self->m_previewTo) {
    // Render transition frame
    auto settings = self->getCurrentSettings();
    auto effect = self->createEffect(settings.effectName);
    auto easingFunc = bwp::transition::Easing::getByName(settings.easingName);
    double easedProgress = easingFunc(self->m_previewProgress);

    if (effect) {
      effect->render(cr, self->m_previewFrom, self->m_previewTo, easedProgress,
                     width, height, {});
    }
  } else if (self->m_previewFrom) {
    // Just show the "from" image
    cairo_set_source_surface(cr, self->m_previewFrom, 0, 0);
    cairo_paint(cr);
  } else {
    // Show placeholder gradient
    cairo_set_source_rgba(cr, 0.3, 0.4, 0.6, 1.0);
    cairo_rectangle(cr, 0, 0, width / 2.0, height);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.6, 0.4, 0.3, 1.0);
    cairo_rectangle(cr, width / 2.0, 0, width / 2.0, height);
    cairo_fill(cr);

    // Text hint
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.7);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);

    cairo_text_extents_t extents;
    const char *text = "Click 'Preview' to test";
    cairo_text_extents(cr, text, &extents);
    cairo_move_to(cr, (width - extents.width) / 2,
                  (height + extents.height) / 2);
    cairo_show_text(cr, text);
  }
}

} // namespace bwp::gui
