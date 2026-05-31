#ifndef CC_SERVER_H
#define CC_SERVER_H

#include "chrono_cache.h"
#include "server/worker.h"
#include <memory>

class ChronoCacheServer {
    public:

    struct ChronoCacheServerConfig {
        int port;
        int num_workers; // auto (hardware concurrency limit)
    };

    ChronoCacheServer(const ChronoCacheServerConfig config, ChronoCache& cache);

    void start();
    void stop();
    void wait();

    private:

    ChronoCacheServerConfig config;
    ChronoCache& cache;
    std::vector<std::unique_ptr<Worker>> workers;

    void setup_signal_handlers();
};

#endif