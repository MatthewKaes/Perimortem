// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/node.hpp"

namespace Puffer::Lsp {

// Semantic tokens classify the retained lexical stream for editor presentation.
// They do not change parsing or attach highlighting facts to the source graph.
auto semantic_legend(Perimortem::Memory::Allocator::Arena& arena)
    -> Perimortem::Serialization::Json::Node;
auto semantic_tokens_for(
    Perimortem::Memory::Allocator::Arena& arena,
    Perimortem::Core::View::Bytes source)
    -> Perimortem::Serialization::Json::Node;

}  // namespace Puffer::Lsp
