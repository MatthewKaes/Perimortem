// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/concepts/bitflag.hpp"
#include "core/memory/managed_string.hpp"

#include "token.hpp"

#include <string>

namespace Tetrodotoxin::Lexical {

// Tokenizer aims to be fast and only takes a view of the data.
// If the ByteView source is destroyed then the parser itself no longer has a
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
  auto parse(const Perimortem::Memory::ManagedString source,
             bool strip_disabled = true) -> void;

  inline constexpr auto get_tokens() const -> const TokenStream& {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (EndOfStream) tokens.
  inline constexpr auto empty() const -> bool { return tokens.size() <= 1; }

  inline constexpr auto get_options() const
      -> const Perimortem::Concepts::BitFlag<TtxState> {
    return options;
  };

  inline constexpr auto get_source() const
      -> Perimortem::Memory::ManagedString {
    return Perimortem::Memory::ManagedString(source);
  };

 private:
  // Output artifacts
  std::string source;
  TokenStream tokens;
  Perimortem::Concepts::BitFlag<TtxState> options;
};

}  // namespace Tetrodotoxin::Lexical
