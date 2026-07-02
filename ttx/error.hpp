// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx {

// Error is source problem data, not presentation.
//
// Parsers can attach the token that failed. Resolution and other source-tree
// owners can report source-level failures without inventing a fake token. The
// caller that owns the source file decides how to render the error.
class Error {
 public:
  Error() = default;
  Error(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source,
      const Lexical::Token& token,
      Count token_index,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : source_path(source_path),
        source(source),
        token(token),
        token_index(token_index),
        message(message),
        hint(hint) {}
  explicit Error(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : source_path(source_path), source(source), message(message), hint(hint) {}

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  constexpr auto get_source() const -> Perimortem::Core::View::Bytes {
    return source;
  }
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
  constexpr auto has_token() const -> Bool {
    return !token.get_text().is_empty();
  }
  constexpr auto is_empty() const -> Bool { return message.is_empty(); }

 private:
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source;
  Lexical::Token token;
  Count token_index = 0;
  Perimortem::Core::View::Bytes message;
  Perimortem::Core::View::Bytes hint;
};

}  // namespace Ttx
