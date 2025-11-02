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
  None = -1,  // Always the first flag for validating.

  ParamTokenizing,
  DisableCommands,

  // Commands
  CppTheme,

  TOTAL_FLAGS,  // Always the last flag for validating.
};

class Tokenizer {
 public:
  Tokenizer(const std::string_view& source, bool strip_disabled = true);

  inline constexpr auto get_tokens() const -> const TokenStream& {
    return tokens;
  };

  inline constexpr auto get_options() const
      -> const Perimortem::Concepts::BitFlag<TtxState> {
    return options;
  };

  inline constexpr auto get_source() const -> const std::string_view& {
    return source;
  };

 private:
  // Output artifacts
  std::string_view source;
  TokenStream tokens;
  Perimortem::Concepts::BitFlag<TtxState> options;
};

}  // namespace Tetrodotoxin::Language::Parser
