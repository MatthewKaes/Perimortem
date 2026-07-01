// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

class Tokenizer {
 public:
  Tokenizer(Perimortem::Memory::Allocator::Arena& arena)
      : tokens(arena), arena(arena) {}

  auto parse(Perimortem::Core::View::Bytes source, Bool strip_disabled = true)
      -> void;

  constexpr auto get_tokens() const -> Perimortem::Core::View::Vector<Token> {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (EndOfStream) tokens.
  constexpr auto is_empty() const -> Bool { return tokens.get_size() <= 1; }

  constexpr auto get_source() const -> const Perimortem::Core::View::Bytes {
    return source;
  };

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  };

 private:
  // Output artifacts
  Perimortem::Core::View::Bytes source;
  Perimortem::Memory::Managed::Vector<Token> tokens;
  Perimortem::Memory::Allocator::Arena& arena;
};

}  // namespace Ttx::Lexical
