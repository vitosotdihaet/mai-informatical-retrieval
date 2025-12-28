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
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "connector.hpp"
#include "log.hpp"
#include "server.hpp"

volatile sig_atomic_t stop_flag = 0;

void signal_handler(int signal)
{
    stop_flag = 1;
}

MinimalAsyncServer::MinimalAsyncServer(int port, int64_t max_response_count, boolean_index::BooleanIndex<std::string> &index) : port_(port), max_response_count_(max_response_count), index_(index), server_fd_(-1), max_fd_(0)
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    FD_ZERO(&master_fds_);
}

MinimalAsyncServer::~MinimalAsyncServer()
{
    stop();
}

bool MinimalAsyncServer::start()
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
    {
        perror("socket");
        return false;
    }

    if (fcntl(server_fd_, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl");
        close(server_fd_);
        return false;
    }

    int yes = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    {
        perror("setsockopt");
        close(server_fd_);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd_);
        return false;
    }

    if (listen(server_fd_, 128) < 0)
    {
        perror("listen");
        close(server_fd_);
        return false;
    }

    FD_SET(server_fd_, &master_fds_);
    max_fd_ = server_fd_;

    spdlog::info("async server listening on port {}", port_);
    return true;
}

#define SEARCH_ENGINE_SERVER_BUFFER_SIZE 4096

/// @brief generate a response for the client request
/// @param client_fd: fd of the client
/// @return boolean: true if client is still alive, false if it was closed
bool MinimalAsyncServer::handle_client_data(int client_fd)
{
    unsigned char buffer[SEARCH_ENGINE_SERVER_BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, SEARCH_ENGINE_SERVER_BUFFER_SIZE, 0);

    if (bytes_read <= 0)
    {
        spdlog::info("client {} disconnected", client_fd);
        close_client(client_fd);
        return false;
    }

    std::vector<unsigned char> request = std::vector<unsigned char>();
    request.reserve(SEARCH_ENGINE_SERVER_BUFFER_SIZE);
    size_t req_offset = 0;

    do
    {
        for (size_t i = 0; i < bytes_read; i++)
        {
            request.push_back(buffer[i]);
        }
        size_t bytes_read = recv(client_fd, buffer, SEARCH_ENGINE_SERVER_BUFFER_SIZE, 0);
    } while (bytes_read == SEARCH_ENGINE_SERVER_BUFFER_SIZE);

    handle_request(client_fd, request);

    return true;
}

void MinimalAsyncServer::handle_request(int client_fd, std::vector<unsigned char> request)
{
    std::string s = std::string(request.begin(), request.end());
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ::isspace));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());

    spdlog::info("client {}: {:?}", client_fd, s);

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    using clock = std::chrono::high_resolution_clock;

    auto start = clock::now();
    auto result = index_.and_query(tokenize_and_stem(s));
    auto end = clock::now();

    spdlog::info("search took {}μs", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

    std::string response;
    response.reserve(1024);
    int64_t response_count = 0;
    for (auto &doc : result)
    {
        if (response_count >= max_response_count_)
        {
            break;
        }

        std::string partial_printing;
        partial_printing.reserve(100);

        for (size_t i = 0; i < doc.size(); i++)
        {
            if (i < 100)
            {
                partial_printing.push_back(doc[i]);
            }
            response.push_back(doc[i]);
        }
        response.push_back('\n');
        spdlog::info("{}", partial_printing);
        response_count++;
    }

    send(client_fd, static_cast<const void *>(response.data()), response.size(), 0);
}

void MinimalAsyncServer::close_client(int client_fd)
{
    close(client_fd);
    FD_CLR(client_fd, &master_fds_);

    auto it = std::find(client_fds_.begin(), client_fds_.end(), client_fd);
    if (it != client_fds_.end())
    {
        std::iter_swap(it, client_fds_.end() - 1);
        client_fds_.pop_back();
    }

    if (client_fd == max_fd_)
    {
        max_fd_ = server_fd_;
        for (int fd : client_fds_)
        {
            if (fd > max_fd_)
                max_fd_ = fd;
        }
    }
}

void MinimalAsyncServer::accept_new_client()
{
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd_, (sockaddr *)&client_addr, &addr_len);

    if (client_fd >= 0)
    {
        fcntl(client_fd, F_SETFL, O_NONBLOCK);
        FD_SET(client_fd, &master_fds_);
        client_fds_.push_back(client_fd);

        if (client_fd > max_fd_)
        {
            max_fd_ = client_fd;
        }

        spdlog::info("new client connected: {}", client_fd);
        const char *welcome = "Welcome to async server!\n";
        send(client_fd, welcome, strlen(welcome), 0);
    }
}

void MinimalAsyncServer::run()
{
    if (!start())
    {
        return;
    }

    spdlog::info("server running. Press Ctrl+C to stop.");

    while (!stop_flag)
    {
        fd_set read_fds = master_fds_;

        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int activity = select(max_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("select");
            break;
        }

        if (FD_ISSET(server_fd_, &read_fds))
        {
            accept_new_client();
        }

        for (size_t i = 0; i < client_fds_.size();)
        {
            int client_fd = client_fds_[i];
            if (FD_ISSET(client_fd, &read_fds))
            {
                if (handle_client_data(client_fd))
                {
                    i++;
                }
            }
            else
            {
                i++;
            }
        }
    }

    spdlog::info("shutting down server...");
}

void MinimalAsyncServer::stop()
{
    for (int client_fd : client_fds_)
    {
        close(client_fd);
    }
    client_fds_.clear();

    if (server_fd_ != -1)
    {
        close(server_fd_);
        server_fd_ = -1;
    }

    FD_ZERO(&master_fds_);
}