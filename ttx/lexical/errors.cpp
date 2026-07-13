// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;

auto Ttx::Lexical::Errors::insert(const Error& error) -> void {
  errors.insert(error);
}

auto Ttx::Lexical::Errors::insert(
    Source source,
    View::Bytes message,
    View::Bytes hint) -> void {
  errors.insert(Error(source.get_path(), source.get_text(), message, hint));
}

auto Ttx::Lexical::Errors::insert(
    const Token& token,
    Source source,
    View::Bytes message,
    View::Bytes hint) -> void {
  errors.insert(
      Error(token, source.get_path(), source.get_text(), message, hint));
}

auto Ttx::Lexical::Errors::insert_range(
    const Token& start,
    const Token& end,
    Source source,
    View::Bytes message,
    View::Bytes hint) -> void {
  errors.insert(
      Error(start, end, source.get_path(), source.get_text(), message, hint));
}
