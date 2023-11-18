#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "Flyer/common/log.h"
#include "Flyer/common/config.h"

void* func(void*) {
    int i = 10;
    while(i--) {
        DEBUGLOG("[Thread] Test Debug log %s", "--DeBUG--");
        INFOLOG("[Thread] Test INFO log %s", "--InFO--");
    }
    return NULL;
}

int main() {
    std::cout << "Test Start...\n";

    Flyer::Config::SetGlobalConfig("/home/os/Desktop/project/config/flyer.xml");
    

    Flyer::Logger::InitGlobalLogger();

    pthread_t thread;
    pthread_create(&thread, NULL, &func, NULL);

    int i = 10;
    while(i--) {
        DEBUGLOG("[Main] Test Debug log %s", "--DeBUG--");
        INFOLOG("[Main] Test log %s", "--INFO--");
    }
    
    pthread_join(thread, NULL);
    // sleep(1);
    return 0;
}