#pragma once
#include <string>

class CommandHandler {
 public:
  static std::string handle(const std::string& raw_input);
};
