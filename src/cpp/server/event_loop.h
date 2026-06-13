#ifndef CC_EVENT_LOOP_H
#define CC_EVENT_LOOP_H

#include <atomic>
#include "../cache/chrono_cache.h"
#include "request_handler/command_handler.h"

class EventLoop {
    public:
    EventLoop(int server_fd, ChronoCache& cache);
    ~EventLoop();

    void run();
    void stop();

    private:

    int server_fd;
    std::atomic<bool> running{false};
    CommandHandler command_handler;
};

#endif