#include "Server.hpp"

int main() {
  Server server(6379);
  server.run();
  return 0;
}
