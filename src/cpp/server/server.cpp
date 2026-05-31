#include "server.h"
#include <memory>

ChronoCacheServer::ChronoCacheServer(ChronoCache& cache): cache(cache) {
    config = ChronoCacheServerConfig();

    workers.push_back(std::make_unique<Worker>(0, config.port, cache)); 
}

void ChronoCacheServer::start() {
    workers[0]->start();
}

void ChronoCacheServer::stop() {
    return;
}

void ChronoCacheServer::wait() {
    return;
}