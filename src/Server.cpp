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
#include <cctype>

constexpr int PORT = 6379;
constexpr int BUFFER_SIZE = 4096;
constexpr int BACKLOG = 5;

// Trim whitespace and convert to uppercase for case-insensitive commands
std::string to_upper(const std::string& s) {
    std::string out;
    for (char c : s) out += std::toupper(c);
    return out;
}

// RESP parsing: converts raw buffer into array of strings (e.g. ["ECHO", "hey"])
bool parse_resp_array(const std::string& input, std::vector<std::string>& out) {
    size_t pos = 0;

    if (input[pos] != '*') return false;
    pos++;

    // Parse array length
    size_t newline = input.find("\r\n", pos);
    if (newline == std::string::npos) return false;

    int num_elements = std::stoi(input.substr(pos, newline - pos));
    pos = newline + 2;

    out.clear();

    for (int i = 0; i < num_elements; ++i) {
        if (input[pos] != '$') return false;
        pos++;

        newline = input.find("\r\n", pos);
        if (newline == std::string::npos) return false;

        int len = std::stoi(input.substr(pos, newline - pos));
        pos = newline + 2;

        if (pos + len > input.size()) return false;

        out.push_back(input.substr(pos, len));
        pos += len + 2; // Skip content and \r\n
    }

    return true;
}

// Build RESP bulk string from text (e.g. "hey" -> $3\r\nhey\r\n)
std::string build_bulk_string(const std::string& s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

void send_response(int client_fd, const std::string& response) {
    send(client_fd, response.c_str(), response.size(), 0);
}

void handle_client_command(int client_fd, const std::string& raw_input) {
    std::vector<std::string> parts;
    if (!parse_resp_array(raw_input, parts)) {
        send_response(client_fd, "-ERR invalid RESP format\r\n");
        return;
    }

    if (parts.empty()) {
        send_response(client_fd, "-ERR empty command\r\n");
        return;
    }

    std::string command = to_upper(parts[0]);

    if (command == "PING") {
        send_response(client_fd, "+PONG\r\n");
    } else if (command == "ECHO") {
        if (parts.size() < 2) {
            send_response(client_fd, "-ERR wrong number of arguments for 'echo'\r\n");
        } else {
            send_response(client_fd, build_bulk_string(parts[1]));
        }
    } else {
        send_response(client_fd, "-ERR unknown command\r\n");
    }
}

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

void accept_new_client(int server_fd, std::vector<pollfd>& fds) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd >= 0) {
        fds.push_back({client_fd, POLLIN, 0});
        std::cout << "New client connected: fd " << client_fd << "\n";
    }
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
                        --i;
                    } else {
                        handle_client_command(fds[i].fd, buffer);
                    }
                }
            }
        }
    }

    for (auto& fd : fds) close(fd.fd);
    return 0;
}
