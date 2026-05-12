// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "rpc_request.hpp"

namespace Tetrodotoxin::Lsp {

class UnixJsonRPC {
 public:
  using DispatchFunc = std::function<RpcResponse(const RpcRequest&)>;
  UnixJsonRPC(const std::string_view& pipe_name);
  ~UnixJsonRPC();

  auto register_method(std::string_view name, DispatchFunc resolver) -> void;
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
  std::unordered_map<std::string_view, DispatchFunc> method_resolver;

  std::atomic<bool> valid = false;
  std::condition_variable cv;
};

}  // namespace Tetrodotoxin::Lsp
