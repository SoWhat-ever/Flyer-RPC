#ifndef FLYER_COMMON_LOG_H
#define FLYER_COMMON_LOG_H

#include "Flyer/common/config.h"
#include "Flyer/common/mutex.h"
#include "Flyer/net/time_event.h"

#include <string>
#include <queue>
#include <memory>
#include <semaphore.h>

// void Print() {
//     printf("11");
// }

namespace Flyer {

#define DEBUGLOG(str, ...) \
  if(Flyer::Logger::GetGlobalLogger()->getLogLevel() <= Flyer::Debug) \
  { \
    Flyer::Logger::GetGlobalLogger()->pushLog(Flyer::LogEvent(Flyer::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + Flyer::formatString(str, ##__VA_ARGS__) + "\n"); \
  } \

#define INFOLOG(str, ...) \
  if(Flyer::Logger::GetGlobalLogger()->getLogLevel() <= Flyer::Info) \
  { \
    Flyer::Logger::GetGlobalLogger()->pushLog(Flyer::LogEvent(Flyer::LogLevel::Info).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + Flyer::formatString(str, ##__VA_ARGS__) + "\n"); \
  } \

#define ERRORLOG(str, ...) \
  if(Flyer::Logger::GetGlobalLogger()->getLogLevel() <= Flyer::Error) \
  { \
    Flyer::Logger::GetGlobalLogger()->pushLog(Flyer::LogEvent(Flyer::LogLevel::Error).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + Flyer::formatString(str, ##__VA_ARGS__) + "\n"); \
  } \


#define APPDEBUGLOG(str, ...) \
  if(Flyer::Logger::GetGlobalLogger()->getLogLevel() <= Flyer::Debug) \
  { \
    Flyer::Logger::GetGlobalLogger()->pushAppLog(Flyer::LogEvent(Flyer::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + Flyer::formatString(str, ##__VA_ARGS__) + "\n"); \
  } \

#define APPINFOLOG(str, ...) \
  if(Flyer::Logger::GetGlobalLogger()->getLogLevel() <= Flyer::Info) \
  { \
    Flyer::Logger::GetGlobalLogger()->pushAppLog(Flyer::LogEvent(Flyer::LogLevel::Info).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + Flyer::formatString(str, ##__VA_ARGS__) + "\n"); \
  } \

#define APPERRORLOG(str, ...) \
  if(Flyer::Logger::GetGlobalLogger()->getLogLevel() <= Flyer::Error) \
  { \
    Flyer::Logger::GetGlobalLogger()->pushAppLog(Flyer::LogEvent(Flyer::LogLevel::Error).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + Flyer::formatString(str, ##__VA_ARGS__) + "\n"); \
  } \

enum LogLevel {
  Unknown = 0,
  Debug = 1,
  Info = 2, 
  Error = 3
};


class AsyncLogger {
public: 
  typedef std::shared_ptr<AsyncLogger> s_ptr;

  AsyncLogger(const std::string& file_name, const std::string& file_path, int max_size);

  void stop();

  void flush();

  void pushLoggerBuffer(std::vector<std::string>& vec);

public:
  static void* Loop(void*);

private:
  std::queue<std::vector<std::string>> m_buffer;

  // m_file_path/m_file_name_yyyymmdd.0
  std::string m_file_name;    // 日志输出文件名字
  std::string m_file_path;    // 日志输出路径
  int m_max_file_size {0};    // 日志单个文件最大大小

  sem_t m_sempahore;

  pthread_t m_thread;

  pthread_cond_t m_condition;
  Mutex m_mutex;

  std::string m_date;            // 当前打印日志的文件日期
  FILE* m_file_hanlder {NULL};   // 当前打开的日志文件句柄

  bool m_reopen_flag {false};

  int m_no {0};   // 日志文件序号

  bool m_stop_flag {false};

};


class Logger {
public:
  typedef std::shared_ptr<Logger> s_ptr;

  Logger(LogLevel level, int type = 1);

  void pushLog(const std::string& msg);

  void pushAppLog(const std::string& msg);

  static Logger* GetGlobalLogger();

  static void InitGlobalLogger(int type = 1);

  void init();

  void log();

  LogLevel getLogLevel() const { return m_set_level; }

  void syncLoop();

private:
  LogLevel m_set_level;

  std::vector<std::string> m_buffer;

  std::vector<std::string> m_app_buffer;

  Mutex m_mutex;

  Mutex m_app_mutex;

  // m_file_path/m_file_name_yyyymmdd.1
  // std::string m_file_name;    // 日志输出文件名字
  // std::string m_file_path;    // 日志输出路径
  // int m_max_file_size {0};    // 日志单个文件最大大小

  AsyncLogger::s_ptr m_async_logger;

  AsyncLogger::s_ptr m_async_app_logger;

  TimerEvent::s_ptr m_timer_event;

  int m_type {0}; // server-async:1  client-sync:0

};


template <typename... Args>
std::string formatString(const char* str, Args&&... args) {
  int size = snprintf(nullptr, 0, str, args...);

  std::string result;
  if(size > 0) {
    result.resize(size);
    snprintf(&result[0], size+1, str, args...);
  }
  return result;
}


std::string LogLevelToString(LogLevel level);


LogLevel StringToLogLevel(const std::string& log_level);


class LogEvent {
public:
  LogEvent(LogLevel level) : m_level(level) {}

  std::string getFileName() {
      return m_file_name;
  }

  LogLevel getLogLevel() {
      return m_level;
  }

  std::string toString();

private:
  std::string m_file_name;
  std::string m_file_line;
  int m_pid;
  int m_tid;
  std::string m_time;
  LogLevel m_level;

};

}

#endif