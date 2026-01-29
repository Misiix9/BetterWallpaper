#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>

namespace bwp::utils {

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, FATAL };

class Logger {
public:
  static void init(const std::filesystem::path &logDir);
  static void log(LogLevel level, const std::string &message,
                  const char *file = __builtin_FILE(),
                  int line = __builtin_LINE());

  // Formatting helper
  template <typename... Args>
  static void logFmt(LogLevel level, const char *fmt, Args... args) {
    char buffer[1024];
    std::snprintf(buffer, sizeof(buffer), fmt, args...);
    log(level, buffer);
  }

private:
  static std::string levelToString(LogLevel level);
  static std::string getTimestamp();
  static void rotateLogsIfNeeded();

  static std::mutex m_mutex;
  static std::filesystem::path m_logFile;
  static std::ofstream m_fileStream;
  static LogLevel m_minLevel;
};

// Macros for convenience
#define LOG_DEBUG(msg)                                                         \
  ::bwp::utils::Logger::log(::bwp::utils::LogLevel::DEBUG, msg, __FILE__,      \
                            __LINE__)
#define LOG_INFO(msg)                                                          \
  ::bwp::utils::Logger::log(::bwp::utils::LogLevel::INFO, msg)
#define LOG_WARN(msg)                                                          \
  ::bwp::utils::Logger::log(::bwp::utils::LogLevel::WARNING, msg)
#define LOG_ERROR(msg)                                                         \
  ::bwp::utils::Logger::log(::bwp::utils::LogLevel::ERROR, msg, __FILE__,      \
                            __LINE__)
#define LOG_FATAL(msg)                                                         \
  ::bwp::utils::Logger::log(::bwp::utils::LogLevel::FATAL, msg, __FILE__,      \
                            __LINE__)

} // namespace bwp::utils
