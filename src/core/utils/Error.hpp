#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
namespace bwp {
enum class ErrorCode : uint32_t {
  Success = 0,
  Unknown = 1,
  NotImplemented = 2,
  InvalidArgument = 3,
  OutOfMemory = 4,
  FileNotFound = 10,
  FileReadError = 11,
  FileWriteError = 12,
  InvalidFormat = 13,
  PermissionDenied = 14,
  DirectoryNotFound = 15,
  WallpaperNotFound = 20,
  UnsupportedWallpaperType = 21,
  WebWallpaperNotSupported = 22,
  RenderError = 23,
  ThumbnailGenerationFailed = 24,
  MonitorNotFound = 30,
  WaylandConnectionFailed = 31,
  LayerShellNotSupported = 32,
  NoDisplayAvailable = 33,
  DaemonNotRunning = 40,
  DBusConnectionFailed = 41,
  DBusMethodFailed = 42,
  IPCTimeout = 43,
  ConfigLoadFailed = 50,
  ConfigSaveFailed = 51,
  ConfigInvalid = 52,
  ProfileNotFound = 53,
  ProfileInvalid = 54,
  WorkshopAPIError = 60,
  DownloadFailed = 61,
  DownloadCancelled = 62,
  NetworkError = 63,
  AuthenticationRequired = 64,
  HyprlandNotRunning = 70,
  HyprlandIPCFailed = 71,
  WorkspaceNotFound = 72,
  ColorExtractionFailed = 80,
  ThemeApplyFailed = 81,
  ThemeToolNotFound = 82,
};
enum class ErrorSeverity {
  Info,     
  Warning,  
  Error,    
  Fatal     
};
class Error {
public:
  explicit Error(ErrorCode code);
  Error(ErrorCode code, std::string_view context);
  Error(ErrorCode code, std::string_view message, std::string_view userMessage,
        ErrorSeverity severity = ErrorSeverity::Error);
  [[nodiscard]] ErrorCode code() const { return m_code; }
  [[nodiscard]] ErrorSeverity severity() const { return m_severity; }
  [[nodiscard]] const std::string &message() const { return m_message; }
  [[nodiscard]] const std::string &userMessage() const { return m_userMessage; }
  [[nodiscard]] const std::optional<std::string> &context() const {
    return m_context;
  }
  [[nodiscard]] std::string toString() const;
  [[nodiscard]] bool isSuccess() const { return m_code == ErrorCode::Success; }
  [[nodiscard]] bool isError() const { return m_code != ErrorCode::Success; }
  explicit operator bool() const { return isError(); }
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
  std::string m_message;      
  std::string m_userMessage;  
  std::optional<std::string> m_context;
  static std::pair<std::string, std::string> getDefaultMessages(ErrorCode code);
  static ErrorSeverity getDefaultSeverity(ErrorCode code);
};
template <typename T> class Result {
public:
  Result(T value) : m_data(std::move(value)) {}
  Result(Error error) : m_data(std::move(error)) {}
  [[nodiscard]] bool hasValue() const {
    return std::holds_alternative<T>(m_data);
  }
  [[nodiscard]] bool hasError() const {
    return std::holds_alternative<Error>(m_data);
  }
  explicit operator bool() const { return hasValue(); }
  [[nodiscard]] T &value() { return std::get<T>(m_data); }
  [[nodiscard]] const T &value() const { return std::get<T>(m_data); }
  [[nodiscard]] Error &error() { return std::get<Error>(m_data); }
  [[nodiscard]] const Error &error() const { return std::get<Error>(m_data); }
  [[nodiscard]] T valueOr(T defaultValue) const {
    if (hasValue())
      return value();
    return defaultValue;
  }
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
}  
