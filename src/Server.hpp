#pragma once

#include <poll.h>

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

class Server {
 public:
  Server(int port);
  void run();

 private:
  int port;
  int server_fd;

  void setup_socket();
  void accept_client(std::vector<pollfd>& fds);
  void handle_client_data(int client_fd, const char* buffer);
  void unblock_if_needed(const std::string& key);

  // Blocking BLPOP state
  std::deque<std::pair<std::string, int>> blpop_wait_queue;  // {key, client_fd}
  std::unordered_map<int, std::string> client_waiting_for;   // client_fd -> key
};
