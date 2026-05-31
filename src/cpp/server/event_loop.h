#ifndef CC_EVENT_LOOP_H
#define CC_EVENT_LOOP_H

#include "../chrono_cache.h"

class EventLoop {
    public:
    EventLoop(int server_fd, ChronoCache& cache);
    ~EventLoop();

    void run();
    void stop();

    private:

    int server_fd;
    ChronoCache& cache;
};

#endif