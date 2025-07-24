#include <chrono>
#include <deque>
#include <string>
#include <unordered_map>
#include <variant>

class Store {
 public:
  static void set(const std::string& key, const std::string& value,
                  long long px_ms = -1);
  static bool get(const std::string& key, std::string& value);

  static int rpush(const std::string& key,
                   const std::vector<std::string>& values);
  static int lpush(const std::string& key,
                   const std::vector<std::string>& values);
  static bool lrange(const std::string& key, int start, int end,
                     std::vector<std::string>& out);
  static int llen(const std::string& key, bool& valid_type);
  static bool lpop(const std::string& key, std::string& out);

 private:
  enum class ValueType { STRING, LIST };

  struct ValueEntry {
    std::variant<std::string, std::deque<std::string>> data;
    std::chrono::steady_clock::time_point expiry;
    bool has_expiry = false;

    ValueType type;
  };

  static std::unordered_map<std::string, ValueEntry> db;
  static bool is_expired(const ValueEntry& entry);
};
