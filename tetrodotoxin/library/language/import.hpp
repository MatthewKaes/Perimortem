// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Import retains one exact Package local route authored by a Library source.
// Package owns route interpretation while Library owns declaration expansion.
class Import {
 public:
  constexpr Import(Perimortem::Core::View::Bytes route) : route(route) {}

  constexpr Import(
      Perimortem::Core::View::Bytes route,
      Ttx::Lexical::Token token,
      Ttx::Lexical::Span span)
      : route(route), token(token), span(span) {}

  // Consumes one complete using statement and retains its route and extent.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Utility::Option<Import>;

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return span; }

 private:
  Perimortem::Core::View::Bytes route;
  Ttx::Lexical::Token token;
  Ttx::Lexical::Span span;
};

}  // namespace Tetrodotoxin::Library::Language
