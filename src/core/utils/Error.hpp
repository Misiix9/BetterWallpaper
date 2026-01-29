#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace bwp {

/**
 * @brief Error codes for the BetterWallpaper application
 */
enum class ErrorCode : uint32_t {
  // General (0-9)
  Success = 0,
  Unknown = 1,
  NotImplemented = 2,
  InvalidArgument = 3,
  OutOfMemory = 4,

  // File operations (10-19)
  FileNotFound = 10,
  FileReadError = 11,
  FileWriteError = 12,
  InvalidFormat = 13,
  PermissionDenied = 14,
  DirectoryNotFound = 15,

  // Wallpaper (20-29)
  WallpaperNotFound = 20,
  UnsupportedWallpaperType = 21,
  WebWallpaperNotSupported = 22,
  RenderError = 23,
  ThumbnailGenerationFailed = 24,

  // Monitor (30-39)
  MonitorNotFound = 30,
  WaylandConnectionFailed = 31,
  LayerShellNotSupported = 32,
  NoDisplayAvailable = 33,

  // IPC/Daemon (40-49)
  DaemonNotRunning = 40,
  DBusConnectionFailed = 41,
  DBusMethodFailed = 42,
  IPCTimeout = 43,

  // Configuration (50-59)
  ConfigLoadFailed = 50,
  ConfigSaveFailed = 51,
  ConfigInvalid = 52,
  ProfileNotFound = 53,
  ProfileInvalid = 54,

  // Workshop (60-69)
  WorkshopAPIError = 60,
  DownloadFailed = 61,
  DownloadCancelled = 62,
  NetworkError = 63,
  AuthenticationRequired = 64,

  // Hyprland (70-79)
  HyprlandNotRunning = 70,
  HyprlandIPCFailed = 71,
  WorkspaceNotFound = 72,

  // Theming (80-89)
  ColorExtractionFailed = 80,
  ThemeApplyFailed = 81,
  ThemeToolNotFound = 82,
};

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
  Info,    // Informational, not a problem
  Warning, // Something unexpected but recoverable
  Error,   // Operation failed but app continues
  Fatal    // Application must exit
};

/**
 * @brief Centralized error class for BetterWallpaper
 *
 * This class encapsulates error information including:
 * - Error code for programmatic handling
 * - Human-readable message for logging
 * - User-friendly message for UI display
 * - Optional context/details
 */
class Error {
public:
  /**
   * @brief Construct an error with just a code (auto-generates messages)
   */
  explicit Error(ErrorCode code);

  /**
   * @brief Construct an error with code and additional context
   */
  Error(ErrorCode code, std::string_view context);

  /**
   * @brief Construct a fully custom error
   */
  Error(ErrorCode code, std::string_view message, std::string_view userMessage,
        ErrorSeverity severity = ErrorSeverity::Error);

  // Getters
  [[nodiscard]] ErrorCode code() const { return m_code; }
  [[nodiscard]] ErrorSeverity severity() const { return m_severity; }
  [[nodiscard]] const std::string &message() const { return m_message; }
  [[nodiscard]] const std::string &userMessage() const { return m_userMessage; }
  [[nodiscard]] const std::optional<std::string> &context() const {
    return m_context;
  }

  /**
   * @brief Get a complete error description for logging
   */
  [[nodiscard]] std::string toString() const;

  /**
   * @brief Check if this is a success (no error)
   */
  [[nodiscard]] bool isSuccess() const { return m_code == ErrorCode::Success; }

  /**
   * @brief Check if this is an error
   */
  [[nodiscard]] bool isError() const { return m_code != ErrorCode::Success; }

  /**
   * @brief Implicit bool conversion (true if error, false if success)
   */
  explicit operator bool() const { return isError(); }

  // Static factory methods
  static Error success() { return Error(ErrorCode::Success); }

  static Error fileNotFound(std::string_view path);
  static Error invalidFormat(std::string_view path,
                             std::string_view expectedFormat);
  static Error webWallpaperNotSupported(std::string_view wallpaperName);
  static Error monitorNotFound(std::string_view monitorName);
  static Error daemonNotRunning();
  static Error configLoadFailed(std::string_view path, std::string_view reason);
  static Error networkError(std::string_view url, std::string_view reason);

private:
  ErrorCode m_code;
  ErrorSeverity m_severity;
  std::string m_message;     // Technical message for logging
  std::string m_userMessage; // User-friendly message for UI
  std::optional<std::string> m_context;

  // Helper to get default messages for error codes
  static std::pair<std::string, std::string> getDefaultMessages(ErrorCode code);
  static ErrorSeverity getDefaultSeverity(ErrorCode code);
};

/**
 * @brief Result type for operations that can fail
 *
 * Usage:
 *   Result<int> divide(int a, int b) {
 *     if (b == 0) return Error(ErrorCode::InvalidArgument, "Division by zero");
 *     return a / b;
 *   }
 *
 *   auto result = divide(10, 2);
 *   if (result.hasValue()) {
 *     std::cout << result.value() << std::endl;
 *   } else {
 *     std::cerr << result.error().message() << std::endl;
 *   }
 */
template <typename T> class Result {
public:
  // Construct with success value
  Result(T value) : m_data(std::move(value)) {}

  // Construct with error
  Result(Error error) : m_data(std::move(error)) {}

  // Check if result contains a value
  [[nodiscard]] bool hasValue() const {
    return std::holds_alternative<T>(m_data);
  }

  // Check if result contains an error
  [[nodiscard]] bool hasError() const {
    return std::holds_alternative<Error>(m_data);
  }

  // Implicit bool conversion (true if has value)
  explicit operator bool() const { return hasValue(); }

  // Get the value (throws if error)
  [[nodiscard]] T &value() { return std::get<T>(m_data); }

  [[nodiscard]] const T &value() const { return std::get<T>(m_data); }

  // Get the error (throws if value)
  [[nodiscard]] Error &error() { return std::get<Error>(m_data); }

  [[nodiscard]] const Error &error() const { return std::get<Error>(m_data); }

  // Get value or default
  [[nodiscard]] T valueOr(T defaultValue) const {
    if (hasValue())
      return value();
    return defaultValue;
  }

  // Map the value with a function
  template <typename F>
  auto map(F &&func) -> Result<decltype(func(std::declval<T>()))> {
    if (hasValue()) {
      return func(value());
    }
    return error();
  }

private:
  std::variant<T, Error> m_data;
};

// Specialization for void (operations that don't return a value)
template <> class Result<void> {
public:
  Result() : m_error(std::nullopt) {}
  Result(Error error) : m_error(std::move(error)) {}

  [[nodiscard]] bool hasValue() const { return !m_error.has_value(); }
  [[nodiscard]] bool hasError() const { return m_error.has_value(); }
  explicit operator bool() const { return hasValue(); }

  [[nodiscard]] const Error &error() const { return m_error.value(); }

  static Result success() { return Result(); }

private:
  std::optional<Error> m_error;
};

} // namespace bwp
