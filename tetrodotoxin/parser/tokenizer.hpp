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
  Colorize,
  ParamTokenizing,

  _PERIMORTEM_ENABLE_BITFLAG(TtxState, uint8_t)
};

class Tokenizer {
 public:
  Tokenizer(const ByteView& source);

  inline constexpr auto get_tokens() const -> const TokenStream& {
    return tokens;
  };

  inline constexpr auto get_options() const -> const TtxStateFlags {
    return options;
  };

  // Get a persistible human view of the token parsing.
  // Mostly used for debugging and testing. Parsing the view is actually vastly
  // slower than tokenizing from source.
  auto dump_tokens() -> std::string;

 private:
  // Output artifacts
  TokenStream tokens;
  TtxStateFlags options;
};

}  // namespace Tetrodotoxin::Language::Parser
