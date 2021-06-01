#ifndef OSIRIS_SRC_LOGGER_H_
#define OSIRIS_SRC_LOGGER_H_

#include <string>

namespace osiris {

enum LogLevel {ERROR = 1, WARNING = 2, INFO = 3, DEBUG = 4};

void SetLogLevel(LogLevel log_level);

class Logger {
 public:
  Logger();
  void LogDebug(const std::string &message, const char* filename,
      int sourceline);
  void LogInfo(const std::string &message);
  void LogWarning(const std::string &message);
  void LogError(const std::string &message);
  void SetLogLevel(LogLevel log_level);

 private:
  LogLevel log_level_;
  void AddTimestamp(std::stringstream &msg_stream);
};

#define LOG_ERROR(msg) osiris::global_logger_instance.LogError(msg)
#define LOG_WARNING(msg) osiris::global_logger_instance.LogWarning(msg)
#define LOG_INFO(msg) osiris::global_logger_instance.LogInfo(msg)
#define LOG_DEBUG(msg) osiris::global_logger_instance.LogDebug(msg,  __FILE__,  __LINE__)

// make global logger instance visible
extern Logger global_logger_instance;

}  // namespace osiris

#endif //OSIRIS_SRC_LOGGER_H_
