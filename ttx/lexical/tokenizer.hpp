// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

// Tokenizer partitions one borrowed byte stream into ordered TTX Tokens. Each
// Token retains coordinates into the source so text remains owned once.
//
// Any byte stream has a token representation. Unrecognized spans receive the
// Unknown Code rather than requiring semantic feedback during tokenization.
class Tokenizer {
 public:
  Tokenizer(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_path)
      : arena(arena), source_text(source_text), source_path(source_path) {
    parse();
  }

  constexpr auto get_tokens() const -> Perimortem::Core::View::Vector<Token> {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (Terminal) tokens.
  constexpr auto is_empty() const -> Bool { return tokens.get_size() <= 1; }

  constexpr auto get_source_text() const
      -> const Perimortem::Core::View::Bytes {
    return source_text;
  };

  constexpr auto get_source_path() const
      -> const Perimortem::Core::View::Bytes {
    return source_path;
  };

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  };

 private:
  auto parse() -> void;

  // Lightweight objects representing the structured view over the arena.
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Vector<Token> tokens;
};

}  // namespace Ttx::Lexical
