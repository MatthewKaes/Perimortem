// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

// An error in some source related context. Commonly used by Dialects during
// parsing.
class Error {
 public:
  Error() = default;
  // Generates a generic error about the source with no logical position.
  Error(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : source_path(source_path),
        source(source),
        message(message),
        hint(hint),
        start_token(nullptr),
        end_token(nullptr) {}

  // Creates an error localized to a singular token.
  Error(
      const Lexical::Token& token,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : source_path(source_path),
        source(source),
        message(message),
        hint(hint),
        start_token(&token),
        end_token(nullptr) {}

  // Creates an error that highlights a range of tokens for issues that cross
  // multiple tokens such as an expression.
  Error(
      const Lexical::Token& start_token,
      const Lexical::Token& end_token,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : source_path(source_path),
        source(source),
        message(message),
        hint(hint),
        start_token(&start_token),
        end_token(&end_token) {}

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  constexpr auto get_source() const -> Perimortem::Core::View::Bytes {
    return source;
  }
  constexpr auto get_start_token() const -> const Lexical::Token* {
    return start_token;
  }
  constexpr auto get_end_token() const -> const Lexical::Token* {
    return end_token;
  }
  constexpr auto get_message() const -> Perimortem::Core::View::Bytes {
    return message;
  }
  constexpr auto get_hint() const -> Perimortem::Core::View::Bytes {
    return hint;
  }
  constexpr auto is_empty() const -> Bool { return message.is_empty(); }

 private:
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source;
  Perimortem::Core::View::Bytes message;
  Perimortem::Core::View::Bytes hint;
  const Lexical::Token* start_token = nullptr;
  const Lexical::Token* end_token = nullptr;
};

}  // namespace Ttx::Lexical
