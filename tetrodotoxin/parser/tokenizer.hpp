// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <string_view>

#include <concepts/bitflag.hpp>

#include "token.hpp"

namespace Tetrodotoxin::Language::Parser {

// Tokenizer aims to be fast and only takes a view of the data.
// If the ByteView source is destroyed then the parser itself no longer has a
// meaningful frame of reference and will cause memory errors.
enum class TtxState : int8_t {
  None = -1, // Always the first flag for validating.

  CppTheme,
  ParamTokenizing,

  TOTAL_FLAGS,  // Always the last flag for validating.
};

class Tokenizer {
 public:
  Tokenizer(const ByteView& source);

  inline constexpr auto get_tokens() const -> const TokenStream& {
    return tokens;
  };

  inline constexpr auto get_options() const
      -> const Perimortem::Concepts::BitFlag<TtxState, uint64_t> {
    return options;
  };

  // Get a persistible human view of the token parsing.
  // Mostly used for debugging and testing. Parsing the view is actually vastly
  // slower than tokenizing from source.
  auto dump_tokens() -> std::string;

 private:
  // Output artifacts
  TokenStream tokens;
  Perimortem::Concepts::BitFlag<TtxState, uint64_t> options;
};

}  // namespace Tetrodotoxin::Language::Parser
