// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/node.hpp"

#include "puffer/lsp/position_encoding.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/token.hpp"

namespace Puffer::Lsp {

// The editor colors the Token stream retained with the current source graph.
// Reading those Tokens and their authored Associations keeps highlighting
// aligned with the strongest hover and navigation facts available.
auto semantic_legend(Perimortem::Memory::Allocator::Arena& arena)
    -> Perimortem::Serialization::Json::Node;
auto semantic_tokens_for(
    Perimortem::Memory::Allocator::Arena& arena,
    Perimortem::Core::View::Bytes source,
    const PositionEncoding& encoding,
    Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens = {},
    const Ttx::Lexical::Associations* associations = nullptr)
    -> Perimortem::Serialization::Json::Node;

}  // namespace Puffer::Lsp
