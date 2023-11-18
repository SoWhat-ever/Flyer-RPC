#ifndef FLYER_COMMON_CONFIG_H
#define FLYER_COMMON_CONFIG_H

#include <map>

namespace Flyer {

class Config {
public:
    Config(const char* xmlfile);

    Config();

    static Config* GetGlobalConfig();

    static void SetGlobalConfig(const char* xmlfile);

public:
    std::string m_log_level;

    std::string m_log_file_name;    // 日志输出文件名字
    std::string m_log_file_path;    // 日志输出路径
    int m_log_max_file_size {0}; 

    int m_log_sync_interval {0};   // 日志同步间隔，ms

    int m_port {0};
    int m_io_threads {0};

    // TiXmlDocument* m_xml_document{NULL};
};

}


#endif