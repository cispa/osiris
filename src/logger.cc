#include <iostream>
#include <sstream>

#include <ctime>
#include <cstring>

#include "logger.h"


namespace osiris {

// global variable for the logger class created in CreateLoggerInstance
Logger global_logger_instance;

Logger::Logger(): log_level_(WARNING) {
}

void SetLogLevel(LogLevel log_level) {
  global_logger_instance.SetLogLevel(log_level);
}

void Logger::SetLogLevel(LogLevel log_level) {
  log_level_ = log_level;
}

void Logger::LogDebug(const std::string &message, const char* filename,
    int sourceline) {
  if (log_level_ >= DEBUG) {
    std::stringstream msg_stream;
    const char base_path[] = "src/";
    // strip filename
    const char* filename_stripped = std::strstr(filename, base_path);
    // change color
    msg_stream << "\033[36m";
    AddTimestamp(msg_stream);
    msg_stream << " DBG(" << filename_stripped << ":" << sourceline << "): "  << message;
    // change color back to normal
    msg_stream << "\033[0m";
    std::cout << msg_stream.str() << std::endl;
  }
}

void Logger::LogInfo(const std::string &message) {
  if (log_level_ >= INFO) {
    std::stringstream msg_stream;
    // change color
    msg_stream << "\033[32m";
    AddTimestamp(msg_stream);
    msg_stream << " INFO: "  << message;
    // change color back to normal
    msg_stream << "\033[0m";
    std::cout << msg_stream.str() << std::endl;
  }
}

void Logger::LogWarning(const std::string &message) {
  if (log_level_ >= WARNING) {
    std::stringstream msg_stream;
    // change color
    msg_stream << "\033[35m";
    AddTimestamp(msg_stream);
    msg_stream << " WARN: "  << message;
    // change color back to normal
    msg_stream << "\033[0m";
    std::cout << msg_stream.str() << std::endl;
  }
}

void Logger::LogError(const std::string &message) {
  if (log_level_ >= ERROR) {
    std::stringstream msg_stream;
    // change color
    msg_stream << "\033[31m";
    AddTimestamp(msg_stream);
    msg_stream << " ERR:  "  << message;
    // change color back to normal
    msg_stream << "\033[0m";
    std::cout << msg_stream.str() << std::endl;
  }
}

void Logger::AddTimestamp(std::stringstream &msg_stream) {
  std::time_t tstamp = std::time(nullptr);
  tm* local_time = std::localtime(&tstamp);
  char tstamp_buffer[256] = {0};
  std::strftime(tstamp_buffer, 256, "%Y:%m:%d-%H:%M:%S", local_time);
  msg_stream << "[";
  msg_stream << tstamp_buffer;
  msg_stream << "]";
}

}  // namespace osiris
