#ifndef CC_EVENT_LOOP_H
#define CC_EVENT_LOOP_H

#include <atomic>
#include "../chrono_cache.h"
#include "request_handler/command_handler.h"
#include "request_handler/resp_parser.h"

class EventLoop {
    public:
    EventLoop(int server_fd, ChronoCache& cache);
    ~EventLoop();

    void run();
    void stop();

    private:

    int server_fd;
    ChronoCache& cache;
    std::atomic<bool> running{false};
    CommandHandler command_handler;
};

#endif