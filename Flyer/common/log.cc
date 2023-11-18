#include "Flyer/common/log.h"
#include "Flyer/common/config.h" 
#include "Flyer/common/util.h"
#include "Flyer/net/eventloop.h"
#include "Flyer/common/run_time.h"

#include <sys/time.h>
#include <ctime>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <stdio.h>

namespace Flyer {

std::string LogLevelToString(LogLevel level) {
  switch (level) {
  case Debug:
    return "DEBUG";
  case Info:
    return "INFO";
  case Error:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

LogLevel StringToLogLevel(const std::string& log_level) {
  if (log_level == "DEBUG")
    return Debug;
  else if (log_level == "INFO")
    return Info;
  else if (log_level == "ERROR")
    return Error;
  else
    return Unknown;
}

// 日志格式：[%y-%m-%d %H:%M:%s.%ms]\t[pid:tid]\t[filename:line]\t[%msg]
std::string LogEvent::toString() {
    struct timeval now_time;

    gettimeofday(&now_time, nullptr);

    struct tm now_time_t;
    localtime_r(&(now_time.tv_sec), &now_time_t);
    
    char buf[128];
    strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);
    std::string time_str(buf);
    int ms = now_time.tv_usec / 1000;
    time_str = time_str + "." + std::to_string(ms);

    int m_pid = getPid();
    int m_thread_id = getThreadId();

    std::stringstream ss;
    ss << "[" << LogLevelToString(m_level) << "]\t"
      << "[" << time_str << "]\t"
      << "[" << m_pid << ":" << m_thread_id << "]\t";
    
    // 获取当前线程处理的请求的 msgid method_name
    std::string msg_id = RunTime::GetRunTime()->m_msgid;
    std::string method_name = RunTime::GetRunTime()->m_method_name;
    if(!msg_id.empty()) {
      ss << "[" << msg_id << "]\t";
    }
    if(!method_name.empty()) {
      ss << "[" << method_name << "]\t";
    }

    return ss.str();
}

static Logger* g_logger = nullptr;

Logger* Logger::GetGlobalLogger() {
    return g_logger;
}

void Logger::InitGlobalLogger(int type /*= 1*/) {
    LogLevel g_logger_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level);
    printf("Init log level [%s]\n", LogLevelToString(g_logger_level).c_str());
    g_logger = new Logger(g_logger_level, type);
    g_logger->init();
}

Logger::Logger(LogLevel level, int type /*= 1*/) : m_set_level(level), m_type(type) {
  if(m_type == 0) {
    return;
  }

  m_async_logger = std::make_shared<AsyncLogger>(
    Config::GetGlobalConfig()->m_log_file_name + "_rpc",
    Config::GetGlobalConfig()->m_log_file_path, 
    Config::GetGlobalConfig()->m_log_max_file_size);
  
  m_async_app_logger = std::make_shared<AsyncLogger>(
    Config::GetGlobalConfig()->m_log_file_name + "_app",
    Config::GetGlobalConfig()->m_log_file_path, 
    Config::GetGlobalConfig()->m_log_max_file_size);
}

void Logger::syncLoop() {
   // 同步 m_buffer 到 async_logger 的buffer队尾
   // printf("sync to async logger\n");
  std::vector<std::string> tmp_vec;
  ScopeMutex<Mutex> lock(m_mutex);
  tmp_vec.swap(m_buffer);
  lock.unlock();

  if(!tmp_vec.empty()) {
    m_async_logger->pushLoggerBuffer(tmp_vec);
  }
  tmp_vec.clear();


  // 同步 m_app_buffer 到 async_app_logger 的buffer队尾
  // printf("sync to async logger\n");
  std::vector<std::string> tmp_app_vec;
  ScopeMutex<Mutex> app_lock(m_app_mutex);
  tmp_app_vec.swap(m_app_buffer);
  app_lock.unlock();

  if(!tmp_app_vec.empty()) {
    m_async_app_logger->pushLoggerBuffer(tmp_app_vec);
  }
}

void Logger::pushLog(const std::string& msg) {
  if(m_type == 0) {
    printf((msg + "\n").c_str());
    return;
  }

  ScopeMutex<Mutex> lock(m_mutex);
  m_buffer.push_back(msg);
  lock.unlock();
}

void Logger::pushAppLog(const std::string& msg) {
  ScopeMutex<Mutex> lock(m_app_mutex);
  m_app_buffer.push_back(msg);
  lock.unlock();
}


void Logger::init() {
  if(m_type == 0) {
    return;
  }

  m_timer_event = std::make_shared<TimerEvent>(Config::GetGlobalConfig()->m_log_sync_interval, true, std::bind(&Logger::syncLoop, this));
  Eventloop::GetCurrentEventloop()->addTimerEvent(m_timer_event);
}

void Logger::log() {
    // ScopeMutex<Mutex> lock(m_mutex);
    // std::queue<std::string> tmp;
    // m_buffer.swap(tmp);
    // lock.unlock();
    // while(!tmp.empty()) {
    //   std::string msg = tmp.front();
    //   tmp.pop();
    //   std::cout << msg.c_str();
    // }
}

AsyncLogger::AsyncLogger(const std::string& file_name, const std::string& file_path, int max_size) 
  : m_file_name(file_name), m_file_path(file_path), m_max_file_size(max_size) {
    
  sem_init(&m_sempahore, 0, 0);

  // pthread_cond_init(&m_condition, NULL);

  pthread_create(&m_thread, NULL, &AsyncLogger::Loop, this);

  sem_wait(&m_sempahore);
}

void* AsyncLogger::Loop(void* arg) {
  // 将 buffer 里面的全部数据打印到文件中，然后线程睡眠，直到有新的数据再重复这个过程
  AsyncLogger* logger = reinterpret_cast<AsyncLogger*>(arg);

  assert(pthread_cond_init(&logger->m_condition, NULL) == 0);

  sem_post(&logger->m_sempahore);

  while(1) {
    ScopeMutex<Mutex> lock(logger->m_mutex);

    while(logger->m_buffer.empty()) {
      pthread_cond_wait(&(logger->m_condition), logger->m_mutex.getMutex());
    }
    // printf("pthread_cond_wait back\n");

    std::vector<std::string> tmp;
    tmp.swap(logger->m_buffer.front());
    logger->m_buffer.pop();

    lock.unlock();

    timeval now;
    gettimeofday(&now, NULL);

    struct tm now_time;
    localtime_r(&(now.tv_sec), &now_time);

    const char* format = "%Y%m%d";
    char date[32];
    strftime(date, sizeof(date), format, &now_time);

    if(std::string(date) != logger->m_date) {
      logger->m_reopen_flag = true;
      logger->m_no = 0;
      logger->m_date = std::string(date);
    }

    if(logger->m_file_hanlder == NULL) {
      logger->m_reopen_flag = true;
    }

    std::stringstream ss;
    ss << logger->m_file_path << logger->m_file_name << "_" << std::string(date) << "_log.";
    std::string log_file_name = ss.str() + std::to_string(logger->m_no);
    // printf("file_name: %s", log_file_name.c_str());

    if(logger->m_reopen_flag) {
      if(logger->m_file_hanlder) {
        fclose(logger->m_file_hanlder);
      }
      logger->m_file_hanlder = fopen(log_file_name.c_str(), "a");
      assert(logger->m_file_hanlder != nullptr);
      logger->m_reopen_flag = false;
    }

    if(ftell(logger->m_file_hanlder) > logger->m_max_file_size) {
      fclose(logger->m_file_hanlder);

      logger->m_no++;
      log_file_name = ss.str() + std::to_string(logger->m_no);
      logger->m_file_hanlder = fopen(log_file_name.c_str(), "a");
      logger->m_reopen_flag = false;
    }

    for(auto &i : tmp) {
      if(!i.empty()) {
        fwrite(i.c_str(), 1, i.length(), logger->m_file_hanlder);
      }
    }
    fflush(logger->m_file_hanlder);

    if(logger->m_stop_flag) {
      return NULL;
    }
  }

  return NULL;
}

void AsyncLogger::stop() {
  m_stop_flag = true;
}

void AsyncLogger::flush() {
  if(m_file_hanlder) {
    fflush(m_file_hanlder);
  }
}

void AsyncLogger::pushLoggerBuffer(std::vector<std::string>& vec) {
  ScopeMutex<Mutex> lock(m_mutex);
  m_buffer.push(vec);
  // 这时候需要唤醒异步日志线程
  pthread_cond_signal(&m_condition);
  // printf("pthread_cond_signal\n");
  lock.unlock();
}


}