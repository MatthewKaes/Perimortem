// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/node.hpp"

namespace Tetrodotoxin::Lsp::Server {

auto semantic_legend(Perimortem::Memory::Allocator::Arena& arena)
    -> Perimortem::Serialization::Json::Node;
auto semantic_tokens_for(
    Perimortem::Memory::Allocator::Arena& arena,
    Perimortem::Core::View::Bytes source)
    -> Perimortem::Serialization::Json::Node;

}  // namespace Tetrodotoxin::Lsp::Server
