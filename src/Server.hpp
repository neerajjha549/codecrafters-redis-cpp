#pragma once
#include <vector>
#include <poll.h>

class Server {
public:
    Server(int port);
    void run();

private:
    int server_fd;
    int port;

    void accept_client(std::vector<pollfd>& fds);
    void handle_client_data(int client_fd, const char* buffer);
    void setup_socket();
};
