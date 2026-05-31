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


void Worker::start() {
    run();
}

void Worker::stop() {
    if(event_loop) {
        event_loop->stop();
    }
}

void Worker::join() {
    return;
}

void Worker::run() {
    int fd = create_socket_and_bind(port);    

    event_loop = std::make_unique<EventLoop>(fd, cache);
    event_loop->run();

    close(fd);
}