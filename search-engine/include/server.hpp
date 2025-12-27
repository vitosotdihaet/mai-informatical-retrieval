#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include <csignal>
#include <spdlog/spdlog.h>

#include "boolean_index.hpp"

class MinimalAsyncServer
{
private:
    int server_fd_;
    int port_;
    fd_set master_fds_;
    int max_fd_;
    std::vector<int> client_fds_;
    boolean_index::BooleanIndex<std::string> &index_;

    int64_t max_response_count_;

public:
    MinimalAsyncServer(int port, int64_t max_response_count, boolean_index::BooleanIndex<std::string> &index);
    ~MinimalAsyncServer();

    bool start();

    /// @brief generate a response for the client request
    /// @param client_fd: fd of the client
    /// @return boolean: true if client is still alive, false if it was closed
    bool handle_client_data(int client_fd);

    void handle_request(int client_fd, std::vector<unsigned char> request);

    void close_client(int client_fd);

    void accept_new_client();

    void run();

    void stop();
};