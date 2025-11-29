// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "storage/formats/rpc_header.hpp"

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

using ManagedString = Perimortem::Memory::ManagedString;
using Arena = Perimortem::Memory::Arena;
using RpcHeader = Perimortem::Storage::Json::RpcHeader;
using DispatchFunc = std::function<
    std::string(Arena&, const ManagedString&, const RpcHeader&)>;

namespace Tetrodotoxin::Lsp {

class UnixJsonRPC {
 public:
  UnixJsonRPC(const std::string& pipe_name);
  ~UnixJsonRPC();

  auto register_method(std::string name, DispatchFunc resolver) -> void;
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