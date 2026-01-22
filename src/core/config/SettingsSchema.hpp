#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace bwp::config {

namespace keys {
// General
const char *const AUTOSTART = "general.autostart";
const char *const AUTOSTART_METHOD =
    "general.autostart_method"; // systemd, xdg, hyprland
const char *const START_MINIMIZED = "general.start_minimized";
const char *const CLOSE_TO_TRAY = "general.close_to_tray";
const char *const CHECK_UPDATES = "general.check_updates";
const char *const LANGUAGE = "general.language";

// Library
const char *const LIBRARY_PATHS = "library.paths";
const char *const SCAN_RECURSIVE = "library.scan_recursive";
const char *const DUPLICATE_HANDLING =
    "library.duplicate_handling"; // ask, older, newer
const char *const AUTO_REMOVE_MISSING = "library.auto_remove_missing";
const char *const THUMBNAIL_SIZE = "library.thumbnail_size";

// Defaults
const char *const DEFAULT_SCALING = "defaults.scaling_mode";
const char *const DEFAULT_AUDIO_ENABLED = "defaults.audio_enabled";
const char *const DEFAULT_VOLUME = "defaults.audio_volume";
const char *const DEFAULT_LOOP = "defaults.loop_enabled";
const char *const DEFAULT_SPEED = "defaults.playback_speed";

// Performance
const char *const FPS_LIMIT = "performance.fps_limit";
const char *const PAUSE_ON_BATTERY = "performance.pause_on_battery";
const char *const PAUSE_ON_FULLSCREEN = "performance.pause_on_fullscreen";
const char *const FULLSCREEN_EXCEPTIONS = "performance.fullscreen_exceptions";
const char *const GPU_ACCELERATION = "performance.gpu_acceleration";

// Transitions
const char *const TRANSITIONS_ENABLED = "transitions.enabled";
const char *const TRANSITIONS_EFFECT = "transitions.default_effect";
const char *const TRANSITIONS_DURATION = "transitions.duration_ms";
const char *const TRANSITIONS_EASING = "transitions.easing";

// Notifications
const char *const NOTIFY_ENABLED = "notifications.enabled";
const char *const NOTIFY_SYSTEM = "notifications.system_notifications";
const char *const NOTIFY_TOASTS = "notifications.in_app_toasts";
const char *const NOTIFY_ON_CHANGE = "notifications.on_wallpaper_change";
const char *const NOTIFY_ON_ERROR = "notifications.on_error";

// Theming
const char *const THEMING_ENABLED = "theming.enabled";
const char *const THEMING_AUTO_APPLY = "theming.auto_apply";
const char *const THEMING_TOOL = "theming.tool";
const char *const THEMING_PALETTE_SIZE = "theming.palette_size";

// Hyprland
const char *const HYPR_WORKSPACE_WALLPAPERS = "hyprland.workspace_wallpapers";
const char *const HYPR_SMOOTH_TRANSITIONS = "hyprland.smooth_transitions";
const char *const HYPR_SPECIAL_WORKSPACE = "hyprland.special_workspace_enabled";

// State
const char *const CURRENT_PROFILE = "current_profile";
} // namespace keys

class SettingsSchema {
public:
  static nlohmann::json getDefaults() {
    return {{"version", "1.0.0"},
            {"general",
             {{"autostart", true},
              {"autostart_method", "systemd"},
              {"start_minimized", true},
              {"close_to_tray", true},
              {"check_updates", true},
              {"language", "en"}}},
            {"library",
             {{"paths", nlohmann::json::array()},
              {"scan_recursive", true},
              {"duplicate_handling", "ask"},
              {"auto_remove_missing", true},
              {"thumbnail_size", 256}}},
            {"defaults",
             {{"scaling_mode", "fill"},
              {"audio_enabled", false},
              {"audio_volume", 50},
              {"loop_enabled", true},
              {"playback_speed", 1.0}}},
            {"performance",
             {{"fps_limit", 60},
              {"pause_on_battery", true},
              {"pause_on_fullscreen", true},
              {"fullscreen_exceptions", nlohmann::json::array()},
              {"gpu_acceleration", true}}},
            {"transitions",
             {{"enabled", true},
              {"default_effect", "expanding_circle"},
              {"duration_ms", 500},
              {"easing", "ease_out"}}},
            {"notifications",
             {{"enabled", true},
              {"system_notifications", true},
              {"in_app_toasts", true},
              {"on_wallpaper_change", false},
              {"on_error", true}}},
            {"theming",
             {{"enabled", true},
              {"auto_apply", true},
              {"tool", "auto"},
              {"palette_size", 16}}},
            {"hyprland",
             {{"workspace_wallpapers", true},
              {"smooth_transitions", true},
              {"special_workspace_enabled", false}}},
            {"current_profile", "default"}};
  }
};

} // namespace bwp::config
