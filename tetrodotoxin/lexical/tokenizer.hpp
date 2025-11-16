// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <string_view>

#include <concepts/bitflag.hpp>

#include "token.hpp"

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
  auto parse(const std::string_view& source, bool strip_disabled = true)
      -> void;

  inline constexpr auto get_tokens() const -> const TokenStream& {
    return tokens;
  };

  // The tokenizer is empty if it has 0 or 1 (EndOfStream) tokens.
  inline constexpr auto empty() const -> bool {
    return tokens.size() <= 1;
  }

  inline constexpr auto get_options() const
      -> const Perimortem::Concepts::BitFlag<TtxState> {
    return options;
  };

  inline constexpr auto get_source() const -> const std::string& {
    return source;
  };

 private:
  // Output artifacts
  std::string source;
  TokenStream tokens;
  Perimortem::Concepts::BitFlag<TtxState> options;
};

}  // namespace Tetrodotoxin::Lexical
