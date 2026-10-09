#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sf {
struct SharedState {
    std::mutex mutex;
    std::string snapshot;
    std::vector<std::string> commands;
};
class HttpServer {
public:
    HttpServer(int port, std::string html, std::string city, SharedState& state);
    ~HttpServer();
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
private:
    int socket_ = -1, port_;
    std::string html_, city_;
    SharedState& state_;
    std::atomic<bool> stopping_{false};
    std::thread thread_;
    void serve();
    void handle(int client);
};
}
