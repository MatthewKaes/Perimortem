// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/thread/worker.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"

#include "tetrodotoxin/lsp/server/rpc/request.hpp"

namespace Tetrodotoxin::Lsp::Server::Rpc {

class Executor {
 public:
  using DispatchFunction = Rpc::Response (*)(const Rpc::Request&);

  static auto register_method(
      Perimortem::Core::View::Bytes name,
      DispatchFunction resolver) -> void;
  static auto execute(Perimortem::Core::View::Bytes pipe_name) -> void;
};

}  // namespace Tetrodotoxin::Lsp::Server::Rpc
