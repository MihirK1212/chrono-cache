#include <cassert>
#include <cerrno>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <poll.h>

#include "connection.h"
#include "event_loop.h"
#include "request_handler/resp_parser.h"
#include "request_handler/resp_serializer.h"

EventLoop::EventLoop(int server_fd, ChronoCache&cache) : server_fd(server_fd), command_handler(cache) {}

EventLoop::~EventLoop() {}

void EventLoop::run() 
{
    /*
        1. fd2conn maintains map from each client connection fd to the Connection object 
        2. using fd2conn and server_fd, create poll_args list -> poll_args[0] = server_pfds, other poll_args[i] = client_pfds
        3. each pfd contains -> {fd, event, revent}
        4. call poll() using the poll_args
        5. use the poll_args[i].revents from each poll_arg after calling poll to handle accordingly
    */

    // map of all client connections, keyed by fd
    std::vector<Connection*> fd2conn;

    // pollfd is the communication template with the poll() call
    // it stores fd, events (request) and revents (response)
    std::vector<struct pollfd> poll_args;

    running = true;
    while (running) {
        poll_args.clear();

        // add server poll fd to poll args list
        // accept() is treated as read() in readiness notifications, so it uses POLLIN
        struct pollfd server_pfd = {server_fd, POLLIN, 0};
        poll_args.push_back(server_pfd);

        // add all client connections to poll args list
        for(Connection* conn: fd2conn) {
            if(!conn) {continue;}

            struct pollfd client_pfd = {conn->fd, 0, 0};

            if(conn->want_read) {
                client_pfd.events |= POLLIN;
            }
            if(conn->want_write) {
                client_pfd.events |= POLLOUT;
            }

            poll_args.push_back(client_pfd);
        }
        
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if(rv < 0 && errno == EINTR) {
            continue; // not an error
        }
        if(rv < 0) {
            throw std::system_error(errno, std::generic_category(), "poll");
        }

        // handle the server socket for new connections
        if(poll_args[0].revents & POLLIN) {
            if(Connection* conn = Connection::handle_accept(server_fd)) {
                if(fd2conn.size() <= (size_t)conn->fd) {
                    fd2conn.resize(conn->fd + 1);
                }
                fd2conn[conn->fd] = conn;
            }
        }

        // handle client connection sockets 
        for(size_t i=1; i < poll_args.size(); i++) {
            uint32_t ready = poll_args[i].revents;
            if(ready == 0) {
                continue;
            }

            Connection* conn = fd2conn[poll_args[i].fd];

            if(ready & POLLIN) {
                assert(conn->want_read);
                std::vector<ReadResponse> requests = conn->handle_read();
                for(const ReadResponse& request: requests) {
                    if(!request.success) {
                        continue;
                    }
                    if(!request.data.has_value()) {
                        continue;
                    }
                    const std::vector<uint8_t>& request_data = request.data.value();
                    std::optional<RespValue> command = RespParser::parse(request_data);
                    std::string resp;
                    if (!command.has_value()) {
                        resp = RespSerializer::error("ERR invalid RESP message");
                    } else {
                        try {
                            resp = command_handler.execute(command.value());
                        } catch (const std::exception& e) {
                            resp = RespSerializer::error(std::string("ERR ") + e.what());
                        }
                    }
                    conn->enqueue_response({
                        reinterpret_cast<const uint8_t*>(resp.data()),
                        reinterpret_cast<const uint8_t*>(resp.data()) + resp.size()
                    });
                }
            }

            if(!conn->want_close && (ready & POLLOUT)) {
                assert(conn->want_write);
                conn->handle_write();
            }


            if ((ready & POLLERR) || (ready & POLLHUP) || conn->want_close) {
                close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete conn;
            }
        }
    }

    // cleanup remaining connections on shutdown
    for (Connection* conn : fd2conn) {
        if (conn) {
            close(conn->fd);
            delete conn;
        }
    }
}

void EventLoop::stop() {
    running = false;
    if (server_fd >= 0) {
        shutdown(server_fd, SHUT_RDWR);
    }
}