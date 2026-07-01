// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Parse {

class Error {
 public:
  Error() = default;
  Error(
      const Lexical::Token& token,
      Count token_index,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      : token(token), token_index(token_index), message(message), hint(hint) {}

  auto get_token() const -> const Lexical::Token&;
  auto get_message() const -> Perimortem::Core::View::Bytes;
  auto get_hint() const -> Perimortem::Core::View::Bytes;
  auto get_token_index() const -> Count;
  auto get_line() const -> Bits_32;
  auto get_column() const -> Bits_32;
  auto is_empty() const -> Bool;

 private:
  Lexical::Token token;
  Count token_index = 0;
  Perimortem::Core::View::Bytes message;
  Perimortem::Core::View::Bytes hint;
};

}  // namespace Ttx::Parse
