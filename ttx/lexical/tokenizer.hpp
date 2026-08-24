// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/lexical/stream.hpp"

namespace Ttx::Lexical {

// Tokenizer partitions one borrowed byte stream into ordered TTX Tokens. Each
// Token retains coordinates into the source so text remains owned once.
//
// Any byte stream has a token representation. Unrecognized spans receive the
// Unknown Code rather than requiring semantic feedback during tokenization.
class Tokenizer : public Stream {
 public:
  Tokenizer(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_path)
      : Stream(arena, source_text, source_path) {
    parse();
  }

  // The tokenizer is empty if it has 0 or 1 (Terminal) tokens.
  constexpr auto is_empty() const -> Bool {
    return get_tokens().get_size() <= 1;
  }

 private:
  auto parse() -> void;
};

}  // namespace Ttx::Lexical
