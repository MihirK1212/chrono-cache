#ifndef CC_SERVER_H
#define CC_SERVER_H

#include "chrono_cache.h"
#include "server/worker.h"
#include <memory>

class ChronoCacheServer {
    public:

    struct ChronoCacheServerConfig {
        int port = 2811;
        int num_workers = 0; // auto (hardware concurrency limit)
    };

    ChronoCacheServer(ChronoCache& cache);

    void start();
    void stop();
    void wait();

    private:

    ChronoCacheServerConfig config;
    ChronoCache& cache;
    std::vector<std::unique_ptr<Worker>> workers;
};

#endif