#include "Logger.hpp"
#include <iostream>
#include <sstream>

namespace bwp::utils {

std::mutex Logger::m_mutex;
std::filesystem::path Logger::m_logFile;
std::ofstream Logger::m_fileStream;
LogLevel Logger::m_minLevel = LogLevel::INFO; // Default

void Logger::init(const std::filesystem::path &logDir) {
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!std::filesystem::exists(logDir)) {
    std::filesystem::create_directories(logDir);
  }

  m_logFile = logDir / "application_debug.log";

  // Simple rotation: if too big, backup and start new
  if (std::filesystem::exists(m_logFile)) {
    if (std::filesystem::file_size(m_logFile) > 5 * 1024 * 1024) { // 5MB
      auto timestamp = std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now());
      std::stringstream ss;
      ss << m_logFile.string() << "."
         << std::put_time(std::localtime(&timestamp), "%Y%m%d%H%M%S");
      std::filesystem::rename(m_logFile, ss.str());
    }
  }

  m_fileStream.open(m_logFile, std::ios::app);
}

void Logger::log(LogLevel level, const std::string &message, const char *file,
                 int line) {
  std::lock_guard<std::mutex> lock(m_mutex);

  // Filter by level? For now log everything essentially unless debug
  // if (level < m_minLevel) return;

  std::string timestamp = getTimestamp();
  std::string levelStr = levelToString(level);

  // Console color codes
  const char *color = "\033[0m";
  switch (level) {
  case LogLevel::DBUG:
    color = "\033[36m";
    break; // Cyan
  case LogLevel::INFO:
    color = "\033[32m";
    break; // Green
  case LogLevel::WARN:
    color = "\033[33m";
    break; // Yellow
  case LogLevel::ERR:
    color = "\033[31m";
    break; // Red
  case LogLevel::FATAL:
    color = "\033[35m";
    break; // Magenta
  }
  const char *reset = "\033[0m";

  // Format: [TIME] [LEVEL] [FILE:LINE] Message
  // Simplify file path
  std::string filename = std::filesystem::path(file).filename().string();

  // File Output
  if (m_fileStream.is_open()) {
    m_fileStream << "[" << timestamp << "] [" << levelStr << "] [" << filename
                 << ":" << line << "] " << message << std::endl;
  }

  // Console Output
  std::cout << color << "[" << timestamp << "] [" << levelStr << "] " << reset
            << message << std::endl;
}

std::string Logger::levelToString(LogLevel level) {
  switch (level) {
  case LogLevel::DBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARN:
    return "WARN";
  case LogLevel::ERR:
    return "ERROR";
  case LogLevel::FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

std::string Logger::getTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

} // namespace bwp::utils
