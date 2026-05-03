#ifndef _FLATEARTH_CORE_LOGGER_HPP
#define _FLATEARTH_CORE_LOGGER_HPP

#include "Defines.hpp"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <print>
#include <queue>
#include <source_location>
#include <string_view>
#include <thread>

namespace febundle::core {

inline const char *COLOR_GREY = "\x1b[90m";
inline const char *COLOR_BLUE = "\x1b[34m";
inline const char *COLOR_GREEN = "\x1b[32m";
inline const char *COLOR_YELLOW = "\x1b[33m";
inline const char *COLOR_RED = "\x1b[31m";
inline const char *COLOR_FATAL = "\x1b[41;97m";
inline const char *COLOR_RESET = "\x1b[0m";

enum class LogLevel {
  Trace = 0,
  Debug,
  Info,
  Warn,
  Error,
  Fatal,
  Off,
};

struct LogMessage {
  LogLevel level;
  std::source_location where;
  string message;
  bool toConsole;
  bool toFile;
};

class Logger {
public:
  inline static Logger &Self() {
    static Logger instance;
    return instance;
  }

  inline void SetLevel(LogLevel lvl) { _level = lvl; }

  inline void SetLogfilePath(const std::filesystem::path &path) { _logPath = path; }

  inline void FileLogging(bool enable) {
    if (enable) {
      if (_logToFile) {
        return;
      }
      _logFile.open(_logPath, std::ios::out | std::ios::app);
      if (!_logFile.is_open()) {
        Log(LogLevel::Error, std::source_location::current(), "Failed to open file");
        _logToFile = false;
        return;
      }
    } else {
      if (_logFile.is_open())
        _logFile.close();
    }
    _logToFile = enable;
  }

  template <typename... Args>
  inline void LogToFile(LogLevel level,
                        std::source_location where,
                        std::string_view fmt,
                        const Args &...args) noexcept {
    if (!_logToFile) {
      Log(LogLevel::Warn,
          std::source_location::current(),
          "cannot log to file if it was not previously enabled");
      return;
    }

    const string out = format(level, where, fmt, args...);
    LogMessage msg{
        .level = level,
        .where = where,
        .message = out,
        .toConsole = true,
        .toFile = true,
    };

    std::lock_guard lock(_qMutex);
    pushToQ(msg);
  }

  template <typename... Args>
  void Log(LogLevel level,
           std::source_location where,
           std::string_view fmt,
           const Args &...args) noexcept {
    if (level < _level) {
      return;
    }

    const string out = format(level, where, fmt, args...);
    LogMessage msg{
        .level = level,
        .where = where,
        .message = out,
        .toConsole = true,
        .toFile = false,
    };

    std::lock_guard lock(_qMutex);
    pushToQ(msg);
  }

private:
  inline Logger() {
    _workerThread = std::thread([this]() {
      while (_running.load()) {
        std::unique_lock lock(_qMutex);
        _cv.wait(lock, [this]() { return !_logQ.empty() || !_running.load(); });

        while (!_logQ.empty()) {
          LogMessage msg = std::move(_logQ.front());
          _logQ.pop();
          lock.unlock();

          if (msg.toConsole) {
            std::print("{}", msg.message);
          }

          if (msg.toFile && _logToFile && _logFile.is_open()) {
            _logFile << msg.message;
          }

          lock.lock();
        }
      }
    });
  }

  ~Logger() {
    _running = false;
    _cv.notify_all();
    if (_workerThread.joinable()) {
      _workerThread.join();
    }

    while (!_logQ.empty()) {
      auto msg = _logQ.front();
      _logQ.pop();
      if (msg.toConsole) {
        std::print("{}", msg.message);
      }

      if (msg.toFile && _logToFile && _logFile.is_open()) {
        _logFile << msg.message;
      }
    }

    if (_logFile.is_open()) {
      _logFile.close();
    }
  }

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  template <typename... Args>
  inline string format(LogLevel level,
                       std::source_location where,
                       std::string_view fmt,
                       const Args &...args) noexcept {
    std::lock_guard<std::mutex> guard(_mutex);
    auto payload = std::vformat(fmt, std::make_format_args(args...));
    const char *color = LevelColour(level);
    const char *reset = COLOR_RESET;
    const char *lvlStr = toString(level);
    const char *full = where.file_name();
    const char *p = std::strstr(full, "src/");
    if (p == nullptr) {
      auto slash = std::strrchr(full, '/');
      p = slash ? slash + 1 : full;
    }

    return std::format("{}[{}] {}:{} in function {}'{}'{}: {}{}\n",
                       color,
                       lvlStr,
                       p,
                       where.line(),
                       reset,
                       where.function_name(),
                       color,
                       payload,
                       reset);
  }

  inline void pushToQ(LogMessage msg) {
    _logQ.push(std::move(msg));
    _cv.notify_one();
  }

  inline constexpr const char *toString(LogLevel lvl) const {
    switch (lvl) {
      case LogLevel::Trace:
        return "TRACE";
      case LogLevel::Debug:
        return "DEBUG";
      case LogLevel::Info:
        return "INFO";
      case LogLevel::Warn:
        return "WARN";
      case LogLevel::Error:
        return "ERROR";
      case LogLevel::Fatal:
        return "FATAL";
      case LogLevel::Off:
        return "";
    }

    // This should never hit
    return "UNKNOWN";
  }

  inline constexpr const char *LevelColour(LogLevel lvl) {
    switch (lvl) {
      case LogLevel::Trace:
        return COLOR_GREEN;
      case LogLevel::Debug:
        return COLOR_BLUE;
      case LogLevel::Info:
        return COLOR_GREEN;
        ;
      case LogLevel::Warn:
        return COLOR_YELLOW;
      case LogLevel::Error:
        return COLOR_RED;
      case LogLevel::Fatal:
        return COLOR_FATAL; // red bg, white text
      default:
        return COLOR_RESET;
    }
  }

private:
  const std::filesystem::path DEFAULT_PATH = "./log.txt";
  std::fstream _logFile;
  bool _logToFile = false;
  std::filesystem::path _logPath = DEFAULT_PATH;
  std::mutex _mutex;
  LogLevel _level = LogLevel::Trace;

  // Async variables
  std::queue<LogMessage> _logQ;
  std::mutex _qMutex;
  std::condition_variable _cv;
  std::thread _workerThread;
  atomic_bool _running{true};
};

} // namespace febundle::core

// —————— macros ——————
#ifndef FE_RELEASE
#define FLATEARTH_LOGGING_ENABLED 1 // Debug/dev build: log everything
#else
#define FLATEARTH_LOGGING_ENABLED 0 // Release: strip low-level logs
#endif

#define FILE_LOGGING(enable) febundle::core::Logger::Self().FileLogging(enable)

// ========== DEBUG BUILD: all logs active ==========

#if FLATEARTH_LOGGING_ENABLED

#define FLOG_SRC_FATAL(fmt, src, ...) \
  febundle::core::Logger::Self().Log(febundle::core::LogLevel::Fatal, src, fmt, ##__VA_ARGS__)

#define LOG_TRACE(fmt, ...)           \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Trace, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)           \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Debug, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...)            \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Info, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...)            \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Warn, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...)           \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Error, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...)           \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Fatal, std::source_location::current(), fmt, ##__VA_ARGS__)

// File logging variants
#define FLOG_TRACE(fmt, ...)                \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Trace, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_DEBUG(fmt, ...)                \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Debug, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_INFO(fmt, ...)                 \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Info, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_WARN(fmt, ...)                 \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Warn, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_ERROR(fmt, ...)                \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Error, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_FATAL(fmt, ...)                \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Fatal, std::source_location::current(), fmt, ##__VA_ARGS__)

// ========== RELEASE BUILD: only WARN / ERROR / FATAL ==========

#else
  // Strip low-level logs entirely
#define LOG_TRACE(...) \
  do {                 \
  } while (0)
#define LOG_DEBUG(...) \
  do {                 \
  } while (0)
#define LOG_INFO(...) \
  do {                \
  } while (0)

// Keep these active
#define LOG_WARN(fmt, ...)            \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Warn, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...)           \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Error, std::source_location::current(), fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...)           \
  febundle::core::Logger::Self().Log( \
      febundle::core::LogLevel::Fatal, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_TRACE(...) \
  do {                  \
  } while (0)
#define FLOG_DEBUG(...) \
  do {                  \
  } while (0)
#define FLOG_INFO(...) \
  do {                 \
  } while (0)

#define FLOG_WARN(fmt, ...)                 \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Warn, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_ERROR(fmt, ...)                \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Error, std::source_location::current(), fmt, ##__VA_ARGS__)

#define FLOG_FATAL(fmt, ...)                \
  febundle::core::Logger::Self().LogToFile( \
      febundle::core::LogLevel::Fatal, std::source_location::current(), fmt, ##__VA_ARGS__)

#endif
#endif // _FLATEARTH_CORE_LOGGER_HPP
