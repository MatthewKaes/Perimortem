// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/access/type.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Import retains one exact Type access authored by a Library source. Package
// contexts expose each segment while Library owns declaration expansion.
class Import {
 public:
  constexpr Import(
      Access::Type type_access,
      Ttx::Lexical::Token token,
      Ttx::Lexical::Span span)
      : type_access(type_access), token(token), span(span) {}

  // Consumes one complete using statement and retains its route and extent.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Utility::Option<Import>;

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return type_access.get_route();
  }

  constexpr auto get_type_access() const -> const Access::Type& {
    return type_access;
  }

  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return span; }

 private:
  Access::Type type_access;
  Ttx::Lexical::Token token;
  Ttx::Lexical::Span span;
};

}  // namespace Tetrodotoxin::Library::Language
