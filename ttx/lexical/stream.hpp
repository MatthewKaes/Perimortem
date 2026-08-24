// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

// Stream is the immutable lexical input shared with Cursor. It keeps source
// storage and classified Tokens together without prescribing how a frontend
// produced them. The TTX Tokenizer fills the same value that a preprocessor or
// another language frontend can provide directly.
class Stream {
 public:
  constexpr Stream(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Vector<Token> tokens = {})
      : arena(arena),
        source_text(source_text),
        source_path(source_path),
        tokens(tokens) {}

  constexpr auto get_tokens() const -> Perimortem::Core::View::Vector<Token> {
    return tokens;
  }

  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

 protected:
  constexpr auto set_tokens(Perimortem::Core::View::Vector<Token> value)
      -> void {
    tokens = value;
  }

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Vector<Token> tokens;
};

}  // namespace Ttx::Lexical
