#ifndef FLYER_COMMON_RUN_TIME_H
#define FLYER_COMMON_RUN_TIME_H

#include <string>

namespace Flyer {

class RunTime {
public:


public:
    static RunTime* GetRunTime();

public:
    std::string m_msgid;

    std::string m_method_name;
};

}

#endif