// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/view/bitflag.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"
#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/memory/view/vector.hpp"

#include "token.hpp"

namespace Tetrodotoxin::Lexical {

// Tokenizer aims to be fast and only takes a view of the data.
// If the View::Byte source is destroyed then the parser itself no longer has a
// meaningful frame of reference and will cause memory errors.
enum class TtxState : Byte {

  ParamTokenizing,
  DisableCommands,

  // Commands
  CppTheme,

  // Enable bit flags.
  TOTAL_FLAGS,
  None = static_cast<Byte>(-1),
};

class Tokenizer {
 public:
  using Tokens = Perimortem::Core::View::Structured<Token>;

  Tokenizer(Perimortem::Memory::Allocator::Arena& arena) : tokens(arena) {}
  
  auto parse(Perimortem::Core::View::Amorphous source,
             Bool strip_disabled = true) -> void;

  inline constexpr auto get_tokens() const
      -> const Tokens {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (EndOfStream) tokens.
  inline constexpr auto empty() const -> Bool { return tokens.get_size() <= 1; }

  inline constexpr auto get_options() const
      -> const Perimortem::Memory::View::BitFlag<TtxState> {
    return options;
  };

  inline constexpr auto get_source() const
      -> const Perimortem::Core::View::Amorphous {
    return source;
  };

 private:
  // Output artifacts
  Perimortem::Core::View::Amorphous source;
  Perimortem::Memory::Managed::Vector<Token> tokens;
  Perimortem::Memory::View::BitFlag<TtxState> options;
};

}  // namespace Tetrodotoxin::Lexical
