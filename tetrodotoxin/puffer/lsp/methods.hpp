// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/puffer/lsp/rpc/executor.hpp"

namespace Tetrodotoxin::Puffer::Lsp {

auto register_methods(Rpc::Executor& executor) -> void;

}  // namespace Tetrodotoxin::Puffer::Lsp
