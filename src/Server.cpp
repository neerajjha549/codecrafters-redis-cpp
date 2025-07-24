#include "Server.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "CommandHandler.hpp"
#include <algorithm>

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
            client_waiting_for.erase(fds[i].fd);  // cleanup if blocked
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
  std::vector<std::string> parts;
  if (!RESP::parse(buffer, parts) || parts.empty()) {
    std::string err = RESP::error("invalid RESP format");
    send(client_fd, err.c_str(), err.size(), 0);
    return;
  }

  std::string cmd = parts[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

  if (cmd == "BLPOP" && parts.size() == 3) {
    std::string key = parts[1];

    std::vector<std::string> popped;
    bool ok = Store::lpop(key, 1, popped);
    if (ok && !popped.empty()) {
      std::string resp = RESP::array({key, popped[0]});
      send(client_fd, resp.c_str(), resp.size(), 0);
    } else if (ok) {
      // block client
      blpop_wait_queue.push_back({key, client_fd});
      client_waiting_for[client_fd] = key;
      std::cout << "Client " << client_fd << " is now blocked on key: " << key
                << "\n";
    } else {
      std::string err = RESP::error(
          "WRONGTYPE Operation against a key holding the wrong kind of value");
      send(client_fd, err.c_str(), err.size(), 0);
    }
    return;
  }

  if (cmd == "RPUSH" && parts.size() >= 3) {
    std::string key = parts[1];
    std::vector<std::string> values(parts.begin() + 2, parts.end());
    int result = Store::rpush(key, values);
    std::string resp = RESP::integer(result);
    send(client_fd, resp.c_str(), resp.size(), 0);

    // Try to unblock client waiting on key
    unblock_if_needed(key);
    return;
  }

  // Fallback to command handler
  std::string response = CommandHandler::handle(buffer);
  send(client_fd, response.c_str(), response.size(), 0);
}

void Server::unblock_if_needed(const std::string& key) {
  for (auto it = blpop_wait_queue.begin(); it != blpop_wait_queue.end(); ++it) {
    if (it->first == key) {
      int client_fd = it->second;

      std::vector<std::string> popped;
      bool ok = Store::lpop(key, 1, popped);

      if (ok && !popped.empty()) {
        std::string response = RESP::array({key, popped[0]});
        send(client_fd, response.c_str(), response.size(), 0);
        std::cout << "Unblocked client " << client_fd << " on key: " << key
                  << "\n";

        client_waiting_for.erase(client_fd);
        blpop_wait_queue.erase(it);
      }
      break;  // only one client gets unblocked
    }
  }
}
