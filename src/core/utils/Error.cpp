#include "Error.hpp"
#include <sstream>
namespace bwp {
Error::Error(ErrorCode code)
    : m_code(code), m_severity(getDefaultSeverity(code)) {
  auto [msg, userMsg] = getDefaultMessages(code);
  m_message = std::move(msg);
  m_userMessage = std::move(userMsg);
}
Error::Error(ErrorCode code, std::string_view context)
    : m_code(code), m_severity(getDefaultSeverity(code)),
      m_context(std::string(context)) {
  auto [msg, userMsg] = getDefaultMessages(code);
  m_message = std::move(msg);
  m_userMessage = std::move(userMsg);
  if (!context.empty()) {
    m_message += ": ";
    m_message += context;
  }
}
Error::Error(ErrorCode code, std::string_view message,
             std::string_view userMessage, ErrorSeverity severity)
    : m_code(code), m_severity(severity), m_message(message),
      m_userMessage(userMessage) {}
std::string Error::toString() const {
  std::ostringstream ss;
  switch (m_severity) {
  case ErrorSeverity::Info:
    ss << "[INFO] ";
    break;
  case ErrorSeverity::Warning:
    ss << "[WARN] ";
    break;
  case ErrorSeverity::Error:
    ss << "[ERROR] ";
    break;
  case ErrorSeverity::Fatal:
    ss << "[FATAL] ";
    break;
  }
  ss << "(" << static_cast<uint32_t>(m_code) << ") ";
  ss << m_message;
  if (m_context.has_value() && !m_context->empty()) {
    ss << " [Context: " << *m_context << "]";
  }
  return ss.str();
}
std::pair<std::string, std::string> Error::getDefaultMessages(ErrorCode code) {
  switch (code) {
  case ErrorCode::Success:
    return {"Operation completed successfully", "Done!"};
  case ErrorCode::Unknown:
    return {"Unknown error occurred",
            "An unexpected error occurred. Please try again."};
  case ErrorCode::NotImplemented:
    return {"Feature not implemented", "This feature is not yet available."};
  case ErrorCode::InvalidArgument:
    return {"Invalid argument provided",
            "Invalid input. Please check your settings."};
  case ErrorCode::OutOfMemory:
    return {"Out of memory", "Not enough memory to complete this operation."};
  case ErrorCode::FileNotFound:
    return {"File not found", "The file could not be found."};
  case ErrorCode::FileReadError:
    return {"Failed to read file",
            "Could not read the file. It may be corrupted or inaccessible."};
  case ErrorCode::FileWriteError:
    return {"Failed to write file",
            "Could not save the file. Check disk space and permissions."};
  case ErrorCode::InvalidFormat:
    return {"Invalid file format", "This file format is not supported."};
  case ErrorCode::PermissionDenied:
    return {"Permission denied",
            "You don't have permission to access this file."};
  case ErrorCode::DirectoryNotFound:
    return {"Directory not found", "The folder could not be found."};
  case ErrorCode::WallpaperNotFound:
    return {"Wallpaper not found", "The wallpaper file could not be found."};
  case ErrorCode::UnsupportedWallpaperType:
    return {"Unsupported wallpaper type",
            "This wallpaper type is not supported."};
  case ErrorCode::WebWallpaperNotSupported:
    return {"Web wallpapers not supported",
            "Web-based Wallpaper Engine wallpapers are not supported. "
            "Please choose a Scene or Video type wallpaper instead."};
  case ErrorCode::RenderError:
    return {"Render error", "Failed to render the wallpaper."};
  case ErrorCode::ThumbnailGenerationFailed:
    return {"Thumbnail generation failed", "Could not create a preview image."};
  case ErrorCode::MonitorNotFound:
    return {"Monitor not found", "The specified monitor was not found."};
  case ErrorCode::WaylandConnectionFailed:
    return {"Wayland connection failed",
            "Could not connect to Wayland display."};
  case ErrorCode::LayerShellNotSupported:
    return {"Layer shell not supported",
            "Your compositor does not support wlr-layer-shell."};
  case ErrorCode::NoDisplayAvailable:
    return {"No display available", "No display is available."};
  case ErrorCode::DaemonNotRunning:
    return {"Daemon not running",
            "BetterWallpaper daemon is not running. Please start it first."};
  case ErrorCode::DBusConnectionFailed:
    return {"D-Bus connection failed",
            "Could not connect to the system message bus."};
  case ErrorCode::DBusMethodFailed:
    return {"D-Bus method call failed",
            "Communication with the daemon failed."};
  case ErrorCode::IPCTimeout:
    return {"IPC timeout", "The operation timed out."};
  case ErrorCode::ConfigLoadFailed:
    return {"Failed to load configuration",
            "Could not load settings. Using defaults."};
  case ErrorCode::ConfigSaveFailed:
    return {"Failed to save configuration", "Could not save your settings."};
  case ErrorCode::ConfigInvalid:
    return {"Invalid configuration", "The configuration file is corrupted."};
  case ErrorCode::ProfileNotFound:
    return {"Profile not found", "The specified profile was not found."};
  case ErrorCode::ProfileInvalid:
    return {"Invalid profile", "The profile configuration is invalid."};
  case ErrorCode::WorkshopAPIError:
    return {"Workshop API error", "Could not communicate with Steam Workshop."};
  case ErrorCode::DownloadFailed:
    return {"Download failed", "The download could not be completed."};
  case ErrorCode::DownloadCancelled:
    return {"Download cancelled", "The download was cancelled."};
  case ErrorCode::NetworkError:
    return {"Network error",
            "A network error occurred. Check your internet connection."};
  case ErrorCode::AuthenticationRequired:
    return {"Authentication required",
            "Please sign in to Steam to access this feature."};
  case ErrorCode::HyprlandNotRunning:
    return {"Hyprland not running", "Hyprland is not running or not detected."};
  case ErrorCode::HyprlandIPCFailed:
    return {"Hyprland IPC failed", "Could not communicate with Hyprland."};
  case ErrorCode::WorkspaceNotFound:
    return {"Workspace not found", "The specified workspace was not found."};
  case ErrorCode::ColorExtractionFailed:
    return {"Color extraction failed",
            "Could not extract colors from the wallpaper."};
  case ErrorCode::ThemeApplyFailed:
    return {"Theme apply failed", "Could not apply the color theme."};
  case ErrorCode::ThemeToolNotFound:
    return {"Theme tool not found",
            "No compatible theming tool (pywal, matugen) was found."};
  }
  return {"Unknown error", "An error occurred."};
}
ErrorSeverity Error::getDefaultSeverity(ErrorCode code) {
  switch (code) {
  case ErrorCode::Success:
    return ErrorSeverity::Info;
  case ErrorCode::NotImplemented:
  case ErrorCode::DownloadCancelled:
  case ErrorCode::ThumbnailGenerationFailed:
    return ErrorSeverity::Warning;
  case ErrorCode::OutOfMemory:
  case ErrorCode::WaylandConnectionFailed:
  case ErrorCode::LayerShellNotSupported:
    return ErrorSeverity::Fatal;
  default:
    return ErrorSeverity::Error;
  }
}
Error Error::fileNotFound(std::string_view path) {
  return Error(ErrorCode::FileNotFound, path);
}
Error Error::invalidFormat(std::string_view path,
                           std::string_view expectedFormat) {
  std::string context =
      std::string(path) + " (expected: " + std::string(expectedFormat) + ")";
  return Error(ErrorCode::InvalidFormat, context);
}
Error Error::webWallpaperNotSupported(std::string_view wallpaperName) {
  return Error(ErrorCode::WebWallpaperNotSupported, wallpaperName);
}
Error Error::monitorNotFound(std::string_view monitorName) {
  return Error(ErrorCode::MonitorNotFound, monitorName);
}
Error Error::daemonNotRunning() { return Error(ErrorCode::DaemonNotRunning); }
Error Error::configLoadFailed(std::string_view path, std::string_view reason) {
  std::string context = std::string(path) + ": " + std::string(reason);
  return Error(ErrorCode::ConfigLoadFailed, context);
}
Error Error::networkError(std::string_view url, std::string_view reason) {
  std::string context = std::string(url) + " - " + std::string(reason);
  return Error(ErrorCode::NetworkError, context);
}
}  
