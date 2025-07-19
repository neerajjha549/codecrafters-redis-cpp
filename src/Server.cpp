#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

constexpr int PORT = 6379;
constexpr int BUFFER_SIZE = 4096;
constexpr int BACKLOG = 5;

// Utility to send a string over a socket
void send_response(int client_fd, const std::string& response) {
    send(client_fd, response.c_str(), response.size(), 0);
}

// Handle a command from the client
void handle_client_message(int client_fd, const std::string& message) {
    std::cout << "Received from fd " << client_fd << ": " << message << "\n";

    if (message.find("PING") != std::string::npos) {
        send_response(client_fd, "+PONG\r\n");
    } else {
        send_response(client_fd, "-ERR unknown command\r\n");
    }
}

// Accept a new client and add it to the poll list
void accept_new_client(int server_fd, std::vector<pollfd>& fds) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd >= 0) {
        fds.push_back({client_fd, POLLIN, 0});
        std::cout << "New client connected: fd " << client_fd << "\n";
    }
}

// Set up the server socket
int setup_server_socket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        close(server_fd);
        exit(1);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port " << PORT << "\n";
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, BACKLOG) != 0) {
        std::cerr << "listen failed\n";
        close(server_fd);
        exit(1);
    }

    return server_fd;
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = setup_server_socket();
    std::cout << "Server listening on port " << PORT << "...\n";

    std::vector<pollfd> fds = {{server_fd, POLLIN, 0}};
    char buffer[BUFFER_SIZE];

    while (true) {
        int poll_count = poll(fds.data(), fds.size(), -1);
        if (poll_count < 0) {
            std::cerr << "poll failed\n";
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    accept_new_client(server_fd, fds);
                } else {
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytesRead = read(fds[i].fd, buffer, BUFFER_SIZE);
                    if (bytesRead <= 0) {
                        std::cout << "Client disconnected: fd " << fds[i].fd << "\n";
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;  // Maintain loop consistency
                    } else {
                        handle_client_message(fds[i].fd, buffer);
                    }
                }
            }
        }
    }

    // Cleanup
    for (auto& fd : fds) {
        close(fd.fd);
    }

    return 0;
}
