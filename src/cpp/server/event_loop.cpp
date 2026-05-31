#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "socket_utils.h"
#include "event_loop.h"
#include "chrono_cache.h"


EventLoop::EventLoop(int server_fd, ChronoCache&cache) : server_fd(server_fd), cache(cache) {}

EventLoop::~EventLoop() {}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        fprintf(stderr, "%s\n", "read() error");
        return;
    }
    fprintf(stderr, "client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

void EventLoop::run() {
    while(true) {
        sockaddr_in client_addr;
        int connfd = accept_connection(server_fd, client_addr);

        do_something(connfd);
        close(connfd);
    }
}

void EventLoop::stop() {
    server_fd = -1;
    return;
}