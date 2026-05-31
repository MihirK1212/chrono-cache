#ifndef CC_SERVER_WORKER_H
#define CC_SERVER_WORKER_H

#include "../chrono_cache.h"
#include "event_loop.h"
#include <memory>
#include <thread>

class Worker {
    public:
    Worker(int id, int port, ChronoCache& cache);
    ~Worker();

    void start();  // Spawns thread
    void stop();   // Signals shutdown
    void join();   // Waits for thread to finish

    private:

    int id;
    int port;
    ChronoCache& cache;

    std::unique_ptr<EventLoop> event_loop;
    std::thread thread;

    void run();
};

#endif