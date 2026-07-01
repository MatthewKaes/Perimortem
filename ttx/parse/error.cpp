// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/error.hpp"

auto Ttx::Parse::Error::get_token() const -> const Lexical::Token& {
  return token;
}

auto Ttx::Parse::Error::get_message() const -> Perimortem::Core::View::Bytes {
  return message;
}

auto Ttx::Parse::Error::get_hint() const -> Perimortem::Core::View::Bytes {
  return hint;
}

auto Ttx::Parse::Error::get_token_index() const -> Count {
  return token_index;
}

auto Ttx::Parse::Error::get_line() const -> Bits_32 {
  return token.get_line();
}

auto Ttx::Parse::Error::get_column() const -> Bits_32 {
  return token.get_column();
}

auto Ttx::Parse::Error::is_empty() const -> Bool {
  return message.is_empty();
}
