// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/lsp/server/rpc/executor.hpp"

namespace Tetrodotoxin::Lsp::Server {

auto register_methods(Rpc::Executor& executor) -> void;

}  // namespace Tetrodotoxin::Lsp::Server
