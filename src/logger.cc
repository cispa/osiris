// Copyright 2021 Daniel Weber, Ahmad Ibrahim, Hamed Nemati, Michael Schwarz, Christian Rossow
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//     limitations under the License.


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
