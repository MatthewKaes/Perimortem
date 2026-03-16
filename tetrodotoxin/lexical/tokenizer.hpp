// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/view/bitflag.hpp"

#include "core/memory/arena.hpp"
#include "core/memory/managed/vector.hpp"
#include "core/memory/view/bytes.hpp"
#include "core/memory/view/vector.hpp"

#include "token.hpp"

namespace Tetrodotoxin::Lexical {

// Tokenizer aims to be fast and only takes a view of the data.
// If the View::Byte source is destroyed then the parser itself no longer has a
// meaningful frame of reference and will cause memory errors.
enum class TtxState : int8_t {
  None = -1,  // Always the first flag for validating.

  ParamTokenizing,
  DisableCommands,

  // Commands
  CppTheme,

  TOTAL_FLAGS,  // Always the last flag for validating.
};

class Tokenizer {
 public:
  using Tokens = Perimortem::Memory::View::Vector<Token>;

  Tokenizer(Perimortem::Memory::Arena& arena) : tokens(arena) {}
  
  auto parse(Perimortem::Memory::View::Bytes source,
             bool strip_disabled = true) -> void;

  inline constexpr auto get_tokens() const
      -> const Tokens {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (EndOfStream) tokens.
  inline constexpr auto empty() const -> bool { return tokens.get_size() <= 1; }

  inline constexpr auto get_options() const
      -> const Perimortem::Concepts::BitFlag<TtxState> {
    return options;
  };

  inline constexpr auto get_source() const
      -> const Perimortem::Memory::View::Bytes {
    return source;
  };

 private:
  // Output artifacts
  Perimortem::Memory::View::Bytes source;
  Perimortem::Memory::Managed::Vector<Token> tokens;
  Perimortem::Concepts::BitFlag<TtxState> options;
};

}  // namespace Tetrodotoxin::Lexical
