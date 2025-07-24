#pragma once
#include <string>

#include "RESP.hpp"
#include "Store.hpp"

class CommandHandler {
 public:
  static std::string handle(const std::string& raw_input);
};
