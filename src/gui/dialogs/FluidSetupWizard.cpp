/*
 * ═══════════════════════════════════════════════════════════════════════════════
 * FLUID SETUP WIZARD - Clean, Minimal, Black & White
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#include "FluidSetupWizard.hpp"
#include "../../core/config/ConfigManager.hpp"
#include "../../core/monitor/MonitorManager.hpp"
#include "../../core/utils/Logger.hpp"
#include "../../core/wallpaper/LibraryScanner.hpp"
#include "../../core/wallpaper/NativeWallpaperSetter.hpp"
#include <cmath>
#include <random>
#include <algorithm>

namespace bwp::gui {

// ═══════════════════════════════════════════════════════════════════════════════
// SPRING PHYSICS - Optimized
// ═══════════════════════════════════════════════════════════════════════════════

SpringValue::SpringValue(double initial, SpringConfig config)
    : m_current(initial), m_target(initial), m_velocity(0.0), m_config(config) {}

void SpringValue::setTarget(double target) { m_target = target; }
void SpringValue::setImmediate(double value) { m_current = value; m_target = value; m_velocity = 0.0; }

bool SpringValue::isAtRest() const {
    return std::abs(m_current - m_target) < 0.01 && std::abs(m_velocity) < 0.01;
}

void SpringValue::tick(double dt) {
    double displacement = m_current - m_target;
    double acceleration = (-m_config.tension * displacement - m_config.friction * m_velocity) / m_config.mass;
    m_velocity += acceleration * dt;
    m_current += m_velocity * dt;
    if (isAtRest()) { m_current = m_target; m_velocity = 0.0; }
}

// ═══════════════════════════════════════════════════════════════════════════════
// KINETIC TEXT - Simplified, elegant
// ═══════════════════════════════════════════════════════════════════════════════

KineticText::KineticText(const std::string& text, double staggerDelay) {
    double delay = 0.0;
    for (char c : text) {
        KineticLetter letter;
        letter.character = c;
        letter.x = SpringValue(0.0, SpringConfig::stiff());
        letter.y = SpringValue(20.0, SpringConfig::wobbly());
        letter.opacity = SpringValue(0.0, SpringConfig::stiff());
        letter.blur = SpringValue(0.0, SpringConfig::stiff());
        letter.scale = SpringValue(0.9, SpringConfig::wobbly());
        letter.rotation = SpringValue(0.0, SpringConfig::stiff());
        letter.delay = delay;
        m_letters.push_back(std::move(letter));
        delay += staggerDelay;
    }
}

void KineticText::setVisible(bool visible) { m_visible = visible; m_time = 0.0; }

void KineticText::tick(double dt) {
    m_time += dt;
    for (auto& letter : m_letters) {
        if (m_time >= letter.delay) {
            if (m_visible) {
                letter.y.setTarget(0.0);
                letter.opacity.setTarget(1.0);
                letter.scale.setTarget(1.0);
            } else {
                letter.y.setTarget(-15.0);
                letter.opacity.setTarget(0.0);
                letter.scale.setTarget(0.95);
            }
        }
        letter.x.tick(dt);
        letter.y.tick(dt);
        letter.opacity.tick(dt);
        letter.scale.tick(dt);
    }
}

void KineticText::render(cairo_t* cr, double baseX, double baseY) {
    cairo_select_font_face(cr, "Inter", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    
    // Pre-compute all letter positions for perfect alignment
    std::vector<double> letterPositions;
    std::vector<cairo_text_extents_t> letterExtents;
    double totalWidth = 0.0;
    
    for (const auto& letter : m_letters) {
        char str[2] = {letter.character, '\0'};
        cairo_text_extents_t ext;
        cairo_text_extents(cr, str, &ext);
        letterPositions.push_back(totalWidth);
        letterExtents.push_back(ext);
        totalWidth += ext.x_advance;
    }
    
    double startX = baseX - totalWidth / 2.0;
    
    for (size_t i = 0; i < m_letters.size(); i++) {
        const auto& letter = m_letters[i];
        if (letter.opacity.getValue() < 0.01) continue;
        
        double opacity = letter.opacity.getValue();
        double yOffset = letter.y.getValue();
        
        // Position: use pre-computed for perfect alignment
        double x = startX + letterPositions[i];
        double y = baseY + yOffset;
        
        cairo_save(cr);
        
        // Smooth white text
        cairo_set_source_rgba(cr, 0.95, 0.95, 0.95, opacity);
        cairo_move_to(cr, x, y);
        
        char str[2] = {letter.character, '\0'};
        cairo_show_text(cr, str);
        
        cairo_restore(cr);
    }
}

bool KineticText::isAtRest() const {
    for (const auto& letter : m_letters) {
        if (!letter.opacity.isAtRest()) return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// PARTICLE SYSTEM - Minimal, white only
// ═══════════════════════════════════════════════════════════════════════════════

void ParticleSystem::emit(double x, double y, int count, double spread) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<> speedDist(30, 100);
    std::uniform_real_distribution<> lifeDist(0.4, 0.8);
    std::uniform_real_distribution<> sizeDist(1, 3);
    
    for (int i = 0; i < count; i++) {
        double angle = angleDist(gen);
        double speed = speedDist(gen) * (spread / 100.0);
        Particle p;
        p.x = x; p.y = y;
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.life = lifeDist(gen);
        p.maxLife = p.life;
        p.size = sizeDist(gen);
        p.opacity = 0.6;
        m_particles.push_back(p);
    }
}

void ParticleSystem::tick(double dt) {
    for (auto& p : m_particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx *= 0.97; p.vy *= 0.97;
        p.life -= dt;
        p.opacity = std::max(0.0, (p.life / p.maxLife) * 0.5);
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
        [](const Particle& p) { return p.life <= 0; }), m_particles.end());
}

void ParticleSystem::render(cairo_t* cr) {
    for (const auto& p : m_particles) {
        cairo_arc(cr, p.x, p.y, p.size, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, p.opacity);
        cairo_fill(cr);
    }
}

void ParticleSystem::clear() { m_particles.clear(); }

// ═══════════════════════════════════════════════════════════════════════════════
// FLUID SETUP WIZARD - Clean Implementation
// ═══════════════════════════════════════════════════════════════════════════════

FluidSetupWizard::FluidSetupWizard(GtkWindow* parent) {
    // Keep GTK alive when no MainWindow exists yet
    GApplication* gapp = g_application_get_default();
    if (gapp) {
        g_application_hold(gapp);
    }
    
    m_window = gtk_window_new();
    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(m_window), parent);
    }
    gtk_window_set_modal(GTK_WINDOW(m_window), TRUE);
    gtk_window_set_title(GTK_WINDOW(m_window), "Setup");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 700, 500);
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_widget_add_css_class(m_window, "fluid-wizard");
    
    setupUi();
    detectMonitors();
}

FluidSetupWizard::~FluidSetupWizard() { stopAnimationLoop(); }

void FluidSetupWizard::setupUi() {
    m_overlay = gtk_overlay_new();
    gtk_window_set_child(GTK_WINDOW(m_window), m_overlay);
    
    setupDrawingArea();
    setupInputWidgets();
    
    GtkEventController* keyController = gtk_event_controller_key_new();
    g_signal_connect(keyController, "key-pressed", G_CALLBACK(onKeyPress), this);
    gtk_widget_add_controller(m_window, keyController);
}

void FluidSetupWizard::setupDrawingArea() {
    m_drawingArea = gtk_drawing_area_new();
    gtk_widget_set_hexpand(m_drawingArea, TRUE);
    gtk_widget_set_vexpand(m_drawingArea, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(m_drawingArea), onDraw, this, nullptr);
    gtk_overlay_set_child(GTK_OVERLAY(m_overlay), m_drawingArea);
}

void FluidSetupWizard::setupInputWidgets() {
    m_inputContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_halign(m_inputContainer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(m_inputContainer, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(m_inputContainer, 120);
    gtk_overlay_add_overlay(GTK_OVERLAY(m_overlay), m_inputContainer);
    
    // Primary button
    m_primaryButton = gtk_button_new_with_label("Get Started");
    gtk_widget_add_css_class(m_primaryButton, "fluid-primary-btn");
    gtk_widget_set_size_request(m_primaryButton, 180, 48);
    g_signal_connect(m_primaryButton, "clicked", G_CALLBACK(onPrimaryClick), this);
    gtk_box_append(GTK_BOX(m_inputContainer), m_primaryButton);
    
    // Secondary button
    m_secondaryButton = gtk_button_new_with_label("Skip");
    gtk_widget_add_css_class(m_secondaryButton, "fluid-secondary-btn");
    gtk_widget_set_visible(m_secondaryButton, FALSE);
    g_signal_connect(m_secondaryButton, "clicked", G_CALLBACK(onSecondaryClick), this);
    gtk_box_append(GTK_BOX(m_inputContainer), m_secondaryButton);
    
    // Path label (shows selected path)
    m_pathEntry = gtk_label_new("");
    gtk_widget_add_css_class(m_pathEntry, "fluid-path-label");
    gtk_widget_set_visible(m_pathEntry, FALSE);
    gtk_box_append(GTK_BOX(m_inputContainer), m_pathEntry);
    
    // Monitor dropdown
    m_monitorDropdown = gtk_drop_down_new(nullptr, nullptr);
    gtk_widget_add_css_class(m_monitorDropdown, "fluid-dropdown");
    gtk_widget_set_size_request(m_monitorDropdown, 250, 40);
    gtk_widget_set_visible(m_monitorDropdown, FALSE);
    gtk_box_append(GTK_BOX(m_inputContainer), m_monitorDropdown);
    
    // Initialize text
    m_titleText = std::make_unique<KineticText>("BetterWallpaper", 0.04);
    m_subtitleText = std::make_unique<KineticText>("", 0.02);
    m_questionText = std::make_unique<KineticText>("", 0.02);
}

void FluidSetupWizard::show() {
    gtk_window_present(GTK_WINDOW(m_window));
    startAnimationLoop();
    m_phase = FluidPhase::Void;
    transitionTo(FluidPhase::Welcome);
}

void FluidSetupWizard::close() {
    stopAnimationLoop();
    
    // Store callback locally - the callback may delete 'this'!
    auto callback = std::move(m_onComplete);
    m_onComplete = nullptr;
    
    // Destroy the wizard window first
    gtk_window_destroy(GTK_WINDOW(m_window));
    
    // Release the application hold
    GApplication* gapp = g_application_get_default();
    if (gapp) {
        g_application_release(gapp);
    }
    
    // Call the callback last (it deletes 'this')
    if (callback) {
        callback();
    }
}

void FluidSetupWizard::startAnimationLoop() {
    if (m_tickSourceId == 0) {
        m_tickSourceId = gtk_widget_add_tick_callback(m_window, onTick, this, nullptr);
        m_lastFrameTime = g_get_monotonic_time();
    }
}

void FluidSetupWizard::stopAnimationLoop() {
    if (m_tickSourceId != 0) {
        gtk_widget_remove_tick_callback(m_window, m_tickSourceId);
        m_tickSourceId = 0;
    }
}

void FluidSetupWizard::transitionTo(FluidPhase newPhase) {
    m_phase = newPhase;
    m_phaseTime = 0.0;
    onPhaseEnter(newPhase);
}

void FluidSetupWizard::onPhaseEnter(FluidPhase phase) {
    // Hide all optional widgets first
    gtk_widget_set_visible(m_secondaryButton, FALSE);
    gtk_widget_set_visible(m_pathEntry, FALSE);
    gtk_widget_set_visible(m_monitorDropdown, FALSE);
    
    switch (phase) {
        case FluidPhase::Welcome:
            m_titleText = std::make_unique<KineticText>("BetterWallpaper", 0.04);
            m_titleText->setVisible(true);
            m_subtitleText = std::make_unique<KineticText>("Your desktop, reimagined.", 0.025);
            m_subtitleText->setVisible(true);
            gtk_button_set_label(GTK_BUTTON(m_primaryButton), "Get Started");
            m_buttonOpacity.setTarget(1.0);
            m_glowIntensity.setTarget(0.15);
            break;
            
        case FluidPhase::MonitorSense:
            m_titleText->setVisible(false);
            m_subtitleText->setVisible(false);
            m_questionText = std::make_unique<KineticText>("Select your display", 0.03);
            m_questionText->setVisible(true);
            gtk_widget_set_visible(m_monitorDropdown, TRUE);
            gtk_widget_set_visible(m_secondaryButton, TRUE);
            gtk_button_set_label(GTK_BUTTON(m_primaryButton), "Continue");
            gtk_button_set_label(GTK_BUTTON(m_secondaryButton), "Use All");
            break;
            
        case FluidPhase::LibraryPulse:
            m_questionText = std::make_unique<KineticText>("Where are your wallpapers?", 0.025);
            m_questionText->setVisible(true);
            gtk_widget_set_visible(m_pathEntry, TRUE);
            gtk_widget_set_visible(m_secondaryButton, TRUE);
            gtk_button_set_label(GTK_BUTTON(m_primaryButton), "Choose Folder");
            gtk_button_set_label(GTK_BUTTON(m_secondaryButton), "Skip");
            gtk_label_set_text(GTK_LABEL(m_pathEntry), "No folder selected");
            break;
            
        case FluidPhase::Completion:
            m_questionText = std::make_unique<KineticText>("You're all set.", 0.04);
            m_questionText->setVisible(true);
            m_particles.emit(350, 250, 40, 120);
            gtk_button_set_label(GTK_BUTTON(m_primaryButton), "Launch");
            m_glowIntensity.setTarget(0.3);
            saveConfiguration();
            break;
            
        case FluidPhase::Dissolve:
            m_questionText->setVisible(false);
            m_glowIntensity.setTarget(0.0);
            gtk_widget_set_visible(m_primaryButton, FALSE);
            g_timeout_add(600, [](gpointer data) -> gboolean {
                static_cast<FluidSetupWizard*>(data)->close();
                return FALSE;
            }, this);
            break;
            
        default: break;
    }
}

void FluidSetupWizard::onPhaseUpdate(FluidPhase, double) {}

void FluidSetupWizard::onTick(double dt) {
    m_time += dt;
    
    // Update springs
    m_glowIntensity.tick(dt);
    m_buttonOpacity.tick(dt);
    m_vignetteIntensity.tick(dt);
    
    // Update text
    if (m_titleText) m_titleText->tick(dt);
    if (m_subtitleText) m_subtitleText->tick(dt);
    if (m_questionText) m_questionText->tick(dt);
    
    m_particles.tick(dt);
    gtk_widget_queue_draw(m_drawingArea);
}

void FluidSetupWizard::render(cairo_t* cr, int width, int height) {
    renderBackground(cr, width, height);
    renderParticles(cr, width, height);
    renderContent(cr, width, height);
    renderGlow(cr, width, height);
    renderVignette(cr, width, height);
}

void FluidSetupWizard::renderBackground(cairo_t* cr, int width, int height) {
    // Pure black background
    cairo_set_source_rgb(cr, 0.04, 0.04, 0.04);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Subtle animated gradient (white/gray only)
    double pulse = (std::sin(m_time * 0.5) + 1.0) * 0.5;
    cairo_pattern_t* gradient = cairo_pattern_create_radial(
        width / 2.0, height / 2.0, 0,
        width / 2.0, height / 2.0, width * 0.6
    );
    cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.1, 0.1, 0.1, 0.3 * pulse);
    cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.04, 0.04, 0.04, 0.0);
    cairo_set_source(cr, gradient);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(gradient);
}

void FluidSetupWizard::renderVignette(cairo_t* cr, int width, int height) {
    cairo_pattern_t* vignette = cairo_pattern_create_radial(
        width / 2.0, height / 2.0, width * 0.25,
        width / 2.0, height / 2.0, width * 0.7
    );
    cairo_pattern_add_color_stop_rgba(vignette, 0.0, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(vignette, 1.0, 0.0, 0.0, 0.0, 0.6);
    cairo_set_source(cr, vignette);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(vignette);
}

void FluidSetupWizard::renderParticles(cairo_t* cr, int, int) {
    m_particles.render(cr);
}

void FluidSetupWizard::renderContent(cairo_t* cr, int width, int height) {
    if (m_titleText) {
        cairo_set_font_size(cr, 42);
        m_titleText->render(cr, width / 2.0, height / 2.0 - 80);
    }
    if (m_subtitleText) {
        cairo_set_font_size(cr, 16);
        m_subtitleText->render(cr, width / 2.0, height / 2.0 - 30);
    }
    if (m_questionText) {
        cairo_set_font_size(cr, 28);
        m_questionText->render(cr, width / 2.0, height / 2.0 - 60);
    }
}

void FluidSetupWizard::renderGlow(cairo_t* cr, int width, int height) {
    double intensity = m_glowIntensity.getValue();
    if (intensity < 0.01) return;
    
    cairo_pattern_t* glow = cairo_pattern_create_radial(
        width / 2.0, height / 2.0, 0,
        width / 2.0, height / 2.0, 200
    );
    cairo_pattern_add_color_stop_rgba(glow, 0.0, 1.0, 1.0, 1.0, intensity * 0.15);
    cairo_pattern_add_color_stop_rgba(glow, 1.0, 1.0, 1.0, 1.0, 0.0);
    cairo_set_source(cr, glow);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(glow);
}

void FluidSetupWizard::positionWidgets(int, int) {}

void FluidSetupWizard::detectMonitors() {
    auto& mm = bwp::monitor::MonitorManager::getInstance();
    auto monitors = mm.getMonitors();
    
    m_detectedMonitors.clear();
    GtkStringList* list = gtk_string_list_new(nullptr);
    gtk_string_list_append(list, "All Displays");
    
    for (const auto& mon : monitors) {
        m_detectedMonitors.push_back(mon.name);
        gtk_string_list_append(list, mon.name.c_str());
    }
    
    if (monitors.empty()) {
        gtk_string_list_append(list, "Default Display");
        m_detectedMonitors.push_back("eDP-1");
    }
    
    gtk_drop_down_set_model(GTK_DROP_DOWN(m_monitorDropdown), G_LIST_MODEL(list));
    g_object_unref(list);
}

void FluidSetupWizard::saveConfiguration() {
    auto& conf = bwp::config::ConfigManager::getInstance();
    if (!m_selectedPath.empty()) {
        conf.set("library.paths", std::vector<std::string>{m_selectedPath});
    }
    conf.set("general.first_run", false);
    conf.save();
    
    if (!m_selectedPath.empty()) {
        bwp::wallpaper::LibraryScanner::getInstance().scan(
            conf.get<std::vector<std::string>>("library.paths"));
    }
}

// GTK Callbacks
void FluidSetupWizard::onDraw(GtkDrawingArea*, cairo_t* cr, int width, int height, gpointer data) {
    static_cast<FluidSetupWizard*>(data)->render(cr, width, height);
}

gboolean FluidSetupWizard::onTick(GtkWidget*, GdkFrameClock* clock, gpointer data) {
    auto* self = static_cast<FluidSetupWizard*>(data);
    gint64 now = gdk_frame_clock_get_frame_time(clock);
    double dt = std::min((now - self->m_lastFrameTime) / 1000000.0, 0.05);
    self->m_lastFrameTime = now;
    self->onTick(dt);
    return G_SOURCE_CONTINUE;
}

void FluidSetupWizard::onPrimaryClick(GtkButton*, gpointer data) {
    auto* self = static_cast<FluidSetupWizard*>(data);
    self->m_particles.emit(350, 350, 15, 60);
    
    switch (self->m_phase) {
        case FluidPhase::Welcome:
            self->transitionTo(FluidPhase::MonitorSense);
            break;
        case FluidPhase::MonitorSense:
            self->transitionTo(FluidPhase::LibraryPulse);
            break;
        case FluidPhase::LibraryPulse: {
            GtkFileDialog* dialog = gtk_file_dialog_new();
            gtk_file_dialog_set_title(dialog, "Select Wallpaper Folder");
            gtk_file_dialog_select_folder(dialog, GTK_WINDOW(self->m_window), nullptr,
                [](GObject* source, GAsyncResult* result, gpointer user_data) {
                    auto* wizard = static_cast<FluidSetupWizard*>(user_data);
                    GError* error = nullptr;
                    GFile* file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
                    if (file) {
                        char* path = g_file_get_path(file);
                        if (path) {
                            wizard->m_selectedPath = path;
                            gtk_label_set_text(GTK_LABEL(wizard->m_pathEntry), path);
                            g_free(path);
                            wizard->transitionTo(FluidPhase::Completion);
                        }
                        g_object_unref(file);
                    }
                    if (error) g_error_free(error);
                }, self);
            g_object_unref(dialog);
            break;
        }
        case FluidPhase::Completion:
            self->transitionTo(FluidPhase::Dissolve);
            break;
        default: break;
    }
}

void FluidSetupWizard::onSecondaryClick(GtkButton*, gpointer data) {
    auto* self = static_cast<FluidSetupWizard*>(data);
    switch (self->m_phase) {
        case FluidPhase::MonitorSense:
            self->transitionTo(FluidPhase::LibraryPulse);
            break;
        case FluidPhase::LibraryPulse:
            self->transitionTo(FluidPhase::Completion);
            break;
        default: break;
    }
}

gboolean FluidSetupWizard::onKeyPress(GtkEventControllerKey*, guint keyval, guint, GdkModifierType, gpointer data) {
    auto* self = static_cast<FluidSetupWizard*>(data);
    if (keyval == GDK_KEY_Escape) {
        self->transitionTo(FluidPhase::Completion);
        return TRUE;
    }
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        onPrimaryClick(nullptr, data);
        return TRUE;
    }
    return FALSE;
}

void FluidSetupWizard::onInputFocusIn(GtkEventControllerFocus*, gpointer) {}
void FluidSetupWizard::onInputFocusOut(GtkEventControllerFocus*, gpointer) {}

} // namespace bwp::gui
