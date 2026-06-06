#ifndef CC_CONNECTION_H
#define CC_CONNECTION_H

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <asm-generic/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/tcp.h> 
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <sys/poll.h>
#include <unistd.h>
#include <optional>

#include "server/socket_utils.h"

const size_t k_max_msg = 32 << 20; 

struct ReadResponse {
    bool success;
    std::optional<std::vector<uint8_t>> data;
};

struct Connection {
    int fd = -1;

    bool want_read = false; // there is something in the connection that needs to be read by the event loop
    bool want_write = false; // something needs to be written to the connection by the event loop
    bool want_close = false;

    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;

    
    std::vector<ReadResponse> handle_read() {
        /*
            Keep filling data into the 'incoming' buffer 
            after every fill, do try_one_request() to see if we have valid request data
            inside each try_one_request, porcess one valid request and then remove its data from the incoming buffer
        */
        uint8_t buf[64*1024];
        ssize_t rv = read(fd, buf, sizeof(buf));
        if (rv < 0 && errno == EAGAIN) {
            return {};
        }
        if (rv == 0) {
            want_close = true;
            return {};
        }
        if (rv < 0) {
            want_close = true;
            return {};
        }
        
        size_t len = (size_t)rv;
        buf_append(incoming, buf, len);

        std::vector<ReadResponse> responses;

        // check if the current data in incoming is enough for a valid request
        // use a while loop to handle multiple requests in a single read
        while(true) {
            ReadResponse response = try_one_request();
            if(!response.success || !response.data.has_value()) {
                break;
            }
            generate_echo_response(response.data.value());
            responses.push_back(response);
        }

        if(outgoing.size() > 0) {
            want_read = false;
            want_write = true;
        }

        return responses;
    }

    void handle_write() {
        assert(outgoing.size() > 0);
        ssize_t rv = write(fd, outgoing.data(), outgoing.size());
        if (rv < 0 && errno == EAGAIN) {
            return; // actually not ready
        }
        if(rv < 0) {
            want_close = true;
            return;
        }

        buf_consume(outgoing, (size_t)rv);

        if(outgoing.size() == 0) {
            want_read = true;
            want_write = false;
        }
    }

    ReadResponse try_one_request() {
        /*
            for now, assume message is of the following format:
            4 bytes -> len of message
            followed by
            'len' bytes -> actual message

            returns true if request data was read completely
            returns false otherwise
        */

        if(incoming.size() < 4) {
            return ReadResponse{false, std::nullopt}; 
        }

        uint32_t len_network = 0;
        memcpy(&len_network, incoming.data(), 4);
        uint32_t len = ntohl(len_network);

        if(len > k_max_msg) {
            want_close = true;
            return ReadResponse{false, std::nullopt};
        }

        if(incoming.size() < (4 + len)) {
            return ReadResponse{false, std::nullopt};  
        }
        const uint8_t *read_data = &incoming[4];
        
        // store the response in a vector
        std::vector<uint8_t> data(len);
        memcpy(data.data(), read_data, len);

        // remove the message from incoming
        buf_consume(incoming, 4 + len);

        return ReadResponse{true, data};  
    }

    void generate_echo_response(const std::vector<uint8_t> &read_data) {
        // generate the response (echo)
        uint32_t len_network_response = htonl(read_data.size());
        buf_append(outgoing, (const uint8_t *)&len_network_response, 4);
        buf_append(outgoing, read_data.data(), read_data.size());
    }

    static Connection* handle_accept(int server_fd) {
        sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        int connfd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if(connfd < 0) {
            return NULL;
        }

        set_nonblocking(connfd);

        Connection* conn = new Connection();
        conn->fd = connfd;
        conn->want_read = true; 
        return conn;
    } 

    static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
        buf.insert(buf.end(), data, data + len);
    }

    static void buf_consume(std::vector<uint8_t> &buf, size_t n) {
        buf.erase(buf.begin(), buf.begin() + n);
    }
};

#endif