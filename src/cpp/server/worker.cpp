#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "worker.h"
#include "server/event_loop.h"
#include "socket_utils.h"

Worker::Worker(int id, int port, ChronoCache&cache): id(id), port(port), cache(cache) {};

Worker::~Worker() {
    if (thread.joinable()) {
        thread.join();
    }
}

void Worker::start() {
    thread = std::thread(&Worker::run, this);
}

void Worker::stop() {
    if(event_loop) {
        event_loop->stop();
    }
}

void Worker::join() {
    thread.join();
}

void Worker::run() {
    try {
        int fd = create_socket_and_bind(port);

        event_loop = std::make_unique<EventLoop>(fd, cache);
        event_loop->run();

        close(fd);
    } catch (const std::exception& e) {
        fprintf(stderr, "Worker %d fatal error: %s\n", id, e.what());
    }
}