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

class MinimalAsyncServer
{
private:
    int server_fd_;
    int port_;
    fd_set master_fds_;
    int max_fd_;
    std::vector<int> client_fds_;

    int64_t max_response_count_;

public:
    MinimalAsyncServer(int port, int64_t max_response_count);
    ~MinimalAsyncServer();

    bool start();

    /// @brief generate a response for the client request
    /// @param client_fd: fd of the client
    /// @return boolean: true if client is still alive, false if it was closed
    bool handleClientData(int client_fd);

    void handleRequest(int client_fd, std::vector<unsigned char> request);

    void closeClient(int client_fd);

    void acceptNewClient();

    void run();

    void stop();
};