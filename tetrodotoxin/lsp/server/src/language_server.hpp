// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "third_party/json.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <thread>
#include <unordered_map>

using json = nlohmann::json;

namespace Tetrodotoxin::Lsp {

class UnixJsonRPC {
 public:
  UnixJsonRPC(const std::string& pipe_name);
  ~UnixJsonRPC();

  auto register_method(
      std::string name,
      std::function<std::string(const json&, const json&, const json&)>
          resolver) -> void;
  auto process() -> void;
  auto shutdown() -> void;

 private:
  static constexpr uint32_t executor_count = 4;

  uint32_t sock_descriptor;
  std::string pipe_name;
  std::thread executors[executor_count];
  std::thread reader;

  std::mutex job_mutex;
  std::list<std::string> job_queue;
  std::unordered_map<
      std::string,
      std::function<std::string(const json&, const json&, const json&)>>
      method_resolver;

  std::atomic<bool> valid = false;
  std::condition_variable cv;
};

}  // namespace Tetrodotoxin::Lsp