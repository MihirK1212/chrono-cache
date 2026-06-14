#include <memory>
#include <signal.h>
#include <thread>

#include "server.h"

ChronoCacheServer::ChronoCacheServer(const ChronoCacheServerConfig config, ChronoCache& cache): config(config), cache(cache) {
    int n = config.num_workers;
    if (n <= 0) {
        n = static_cast<int>(std::thread::hardware_concurrency());
        if (n <= 0) n = 1;
    }

    for (int i = 0; i < n; i++) {
        workers.push_back(std::make_unique<Worker>(i, config.port, cache));
    }

    setup_signal_handlers();
}

void ChronoCacheServer::start() {
    for (auto& w : workers) {
        w->start();
    }
}

void ChronoCacheServer::stop() {
    for (auto& w : workers) {
        w->stop();
    }
}

void ChronoCacheServer::wait() {
    for (auto& w : workers) {
        w->join();
    }
}

// stores the global server pointer for signal handler
// when a terminate signal is received, the server is stopped using this pointer and the signal handler
static ChronoCacheServer* g_server = nullptr;

void signal_handler(int sig) {
    fprintf(stderr, "Received signal %d, stopping server\n", sig);
    if (g_server) {
        g_server->stop();
    }
}


void ChronoCacheServer::setup_signal_handlers() {
    g_server = this; 

    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    
    // instead of terminating the process on SIGPIPE, ignore it
    // SIGPIPE occurs when a write operation is attempted by server on a closed pipe or socket (client disconnected)
    // in this case, we want to ignore it and continue the server
    signal(SIGPIPE, SIG_IGN); 
}