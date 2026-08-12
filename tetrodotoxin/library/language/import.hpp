// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Import retains one exact Type access authored by a Library source. Package
// contexts expose each segment while Library owns declaration expansion.
class Import {
 public:
  constexpr Import(TypeReference type_reference, Ttx::Lexical::Span span)
      : type_reference(type_reference), span(span) {}

  // Consumes one complete using statement and retains its route and extent.
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Import>;

  constexpr auto matches(const Import& other) const -> Bool {
    return type_reference.matches_route(other.type_reference);
  }

  constexpr auto get_type_reference() const -> const TypeReference& {
    return type_reference;
  }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return span; }

 private:
  TypeReference type_reference;
  Ttx::Lexical::Span span;
};

}  // namespace Tetrodotoxin::Library::Language
