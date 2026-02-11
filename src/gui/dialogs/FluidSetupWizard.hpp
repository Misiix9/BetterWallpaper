#pragma once
#include <adwaita.h>
#include <gtk/gtk.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
namespace bwp::gui {
struct SpringConfig {
    double mass = 1.0;        
    double tension = 170.0;   
    double friction = 26.0;   
    static SpringConfig gentle() { return {1.0, 120.0, 14.0}; }
    static SpringConfig wobbly() { return {1.0, 180.0, 12.0}; }
    static SpringConfig stiff()  { return {1.0, 210.0, 20.0}; }
    static SpringConfig molasses() { return {1.0, 280.0, 60.0}; }
};
class SpringValue {
public:
    SpringValue(double initial = 0.0, SpringConfig config = {});
    void setTarget(double target);
    void setImmediate(double value);
    double getValue() const { return m_current; }
    double getVelocity() const { return m_velocity; }
    bool isAtRest() const;
    void tick(double dt);  
private:
    double m_current;
    double m_target;
    double m_velocity;
    SpringConfig m_config;
};
enum class FluidPhase {
    Void,            
    Emergence,       
    Welcome,         
    MonitorSense,    
    LibraryPulse,    
    Completion,      
    Dissolve         
};
struct KineticLetter {
    char character;
    SpringValue x;
    SpringValue y;
    SpringValue opacity;
    SpringValue blur;
    SpringValue scale;
    SpringValue rotation;
    double delay;
};
class KineticText {
public:
    KineticText(const std::string& text, double staggerDelay = 0.03);
    void setVisible(bool visible);
    void setPosition(double x, double y);
    void tick(double dt);
    void render(cairo_t* cr, double baseX, double baseY);
    bool isAtRest() const;
private:
    std::vector<KineticLetter> m_letters;
    bool m_visible = false;
    double m_time = 0.0;
};
struct Particle {
    double x, y;
    double vx, vy;
    double life;
    double maxLife;
    double size;
    double opacity;
};
class ParticleSystem {
public:
    void emit(double x, double y, int count, double spread = 100.0);
    void tick(double dt);
    void render(cairo_t* cr);
    void clear();
private:
    std::vector<Particle> m_particles;
};
class FluidSetupWizard {
public:
    FluidSetupWizard(GtkWindow* parent);
    ~FluidSetupWizard();
    void show();
    void close();
    using CompletionCallback = std::function<void()>;
    void setOnComplete(CompletionCallback callback) { m_onComplete = callback; }
private:
    GtkWidget* m_window;
    GtkWidget* m_drawingArea;
    GtkWidget* m_overlay;
    GtkWidget* m_inputContainer;
    guint m_tickSourceId = 0;
    gint64 m_lastFrameTime = 0;
    double m_time = 0.0;
    FluidPhase m_phase = FluidPhase::Void;
    double m_phaseTime = 0.0;
    SpringValue m_cameraX{0.0, SpringConfig::gentle()};
    SpringValue m_cameraY{0.0, SpringConfig::gentle()};
    SpringValue m_cameraZoom{1.0, SpringConfig::gentle()};
    SpringValue m_cameraRotation{0.0, SpringConfig::gentle()};
    SpringValue m_backgroundHue{220.0, SpringConfig::molasses()};
    SpringValue m_vignetteIntensity{0.8, SpringConfig::gentle()};
    SpringValue m_glowIntensity{0.0, SpringConfig::wobbly()};
    std::unique_ptr<KineticText> m_titleText;
    std::unique_ptr<KineticText> m_subtitleText;
    std::unique_ptr<KineticText> m_questionText;
    ParticleSystem m_particles;
    GtkWidget* m_primaryButton;
    GtkWidget* m_secondaryButton;
    GtkWidget* m_pathEntry;
    GtkWidget* m_monitorDropdown;
    SpringValue m_buttonOpacity{0.0, SpringConfig::stiff()};
    SpringValue m_buttonY{100.0, SpringConfig::wobbly()};
    SpringValue m_inputFocusDim{0.0, SpringConfig::gentle()};
    std::string m_selectedPath;
    std::vector<std::string> m_detectedMonitors;
    CompletionCallback m_onComplete;
    void setupUi();
    void setupDrawingArea();
    void setupInputWidgets();
    void startAnimationLoop();
    void stopAnimationLoop();
    void transitionTo(FluidPhase newPhase);
    void onPhaseEnter(FluidPhase phase);
    void onPhaseUpdate(FluidPhase phase, double dt);
    void onTick(double dt);
    void render(cairo_t* cr, int width, int height);
    void renderBackground(cairo_t* cr, int width, int height);
    void renderVignette(cairo_t* cr, int width, int height);
    void renderParticles(cairo_t* cr, int width, int height);
    void renderContent(cairo_t* cr, int width, int height);
    void renderGlow(cairo_t* cr, int width, int height);
    void positionWidgets(int width, int height);
    void detectMonitors();
    void saveConfiguration();
    static void onDraw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data);
    static gboolean onTick(GtkWidget* widget, GdkFrameClock* clock, gpointer data);
    static void onPrimaryClick(GtkButton* btn, gpointer data);
    static void onSecondaryClick(GtkButton* btn, gpointer data);
    static gboolean onKeyPress(GtkEventControllerKey* controller, guint keyval, 
                                guint keycode, GdkModifierType state, gpointer data);
    static void onInputFocusIn(GtkEventControllerFocus* controller, gpointer data);
    static void onInputFocusOut(GtkEventControllerFocus* controller, gpointer data);
};
}  
