// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "storage/formats/json.hpp"

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

using Node = Perimortem::Storage::Json::Node;
using ManagedString = Perimortem::Memory::ManagedString;

namespace Tetrodotoxin::Lsp {

class UnixJsonRPC {
 public:
  UnixJsonRPC(const std::string& pipe_name);
  ~UnixJsonRPC();

  auto register_method(
      std::string name,
      std::function<std::string(const ManagedString&, uint32_t, const Node&)>
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
      std::string_view,
      std::function<std::string(ManagedString, uint32_t, const Node&)>>
      method_resolver;

  std::atomic<bool> valid = false;
  std::condition_variable cv;
};

}  // namespace Tetrodotoxin::Lsp