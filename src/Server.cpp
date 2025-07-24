#include "Server.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "CommandHandler.hpp"

constexpr int BACKLOG = 5;
constexpr int BUFFER_SIZE = 4096;

Server::Server(int port) : port(port), server_fd(-1) {}

void Server::setup_socket() {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  int reuse = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  bind(server_fd, (sockaddr*)&addr, sizeof(addr));
  listen(server_fd, BACKLOG);
}

void Server::run() {
  setup_socket();
  std::cout << "Server listening on port " << port << "...\n";

  std::vector<pollfd> fds = {{server_fd, POLLIN, 0}};
  char buffer[BUFFER_SIZE];

  while (true) {
    poll(fds.data(), fds.size(), -1);
    for (size_t i = 0; i < fds.size(); ++i) {
      if (fds[i].revents & POLLIN) {
        if (fds[i].fd == server_fd) {
          accept_client(fds);
        } else {
          memset(buffer, 0, BUFFER_SIZE);
          int bytes = read(fds[i].fd, buffer, BUFFER_SIZE);
          if (bytes <= 0) {
            close(fds[i].fd);
            fds.erase(fds.begin() + i);
            --i;
          } else {
            handle_client_data(fds[i].fd, buffer);
          }
        }
      }
    }
  }
}

void Server::accept_client(std::vector<pollfd>& fds) {
  sockaddr_in client{};
  socklen_t len = sizeof(client);
  int client_fd = accept(server_fd, (sockaddr*)&client, &len);
  fds.push_back({client_fd, POLLIN, 0});
  std::cout << "Client connected: fd " << client_fd << "\n";
}

void Server::handle_client_data(int client_fd, const char* buffer) {
  std::string response = CommandHandler::handle(buffer);
  send(client_fd, response.c_str(), response.size(), 0);
}
