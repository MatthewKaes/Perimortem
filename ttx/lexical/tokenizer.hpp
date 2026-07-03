// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

// Tokenizer allows for preprocessing a raw byte stream into a stream of TTX.
// The tokens are applied like any other layer and only enrich the TTX IR by
// precaculating each tokens mapping to the limited TTX representational model
// which is extremely useful context for parsers.
//
// Typically used with ASCII or UTF text, but since any arbitrary byte stream
// has a canonical TTX representation in the data model this layer produces no
// real diagnostic information. Dialect layers are typically the first layers to
// start reasoning about the input.
class Tokenizer {
 public:
  Tokenizer(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_name,
      Bool strip_disabled = True)
      : arena(arena) {
    parse(strip_disabled);
  }

  constexpr auto get_tokens() const -> Perimortem::Core::View::Vector<Token> {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (EndOfStream) tokens.
  constexpr auto is_empty() const -> Bool { return tokens.get_size() <= 1; }

  constexpr auto get_source_text() const
      -> const Perimortem::Core::View::Bytes {
    return source_text;
  };

  constexpr auto get_source_name() const
      -> const Perimortem::Core::View::Bytes {
    return source_name;
  };

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  };

 private:
  auto parse(Bool strip_disabled = true) -> void;

  // All light weight objects that represent the structured view over the arena.
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes source_name;
  Perimortem::Core::View::Vector<Token> tokens;
};

}  // namespace Ttx::Lexical
