// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Parse {

// Parse error is diagnostic data, not presentation.
//
// The parser records the token, token index, message, and an optional
// remediation hint for a parse problem. The shape is deliberately general.
// Nothing here depends on source envelope syntax, so dialect parsers can use
// the same error record without inventing parallel diagnostic facts.
//
// Rendering, logging, and transport belong to the user facing surface that
// consumes the error. The CLI can render colored source excerpts, the LSP can
// publish structured diagnostics, and tests can inspect the raw fact without
// going through terminal formatting.
class Error {
 public:
  Error() = default;
  Error(
      const Lexical::Token& token,
      Count token_index,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : token(token), token_index(token_index), message(message), hint(hint) {}

  constexpr auto get_token() const -> const Lexical::Token& { return token; }
  constexpr auto get_message() const -> Perimortem::Core::View::Bytes {
    return message;
  }
  constexpr auto get_hint() const -> Perimortem::Core::View::Bytes {
    return hint;
  }
  constexpr auto get_token_index() const -> Count { return token_index; }
  constexpr auto get_line() const -> Bits_32 { return token.get_line(); }
  constexpr auto get_column() const -> Bits_32 { return token.get_column(); }

  // Default constructed errors are empty sentinels. Parse success is still
  // determined by the cursor error list, not by asking a source object or AST
  // node whether it is valid.
  constexpr auto is_empty() const -> Bool { return message.is_empty(); }

 private:
  Lexical::Token token;
  Count token_index = 0;
  Perimortem::Core::View::Bytes message;
  Perimortem::Core::View::Bytes hint;
};

}  // namespace Ttx::Parse
