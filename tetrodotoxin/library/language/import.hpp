// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Import retains one exact contextual route authored by a Library source. At
// link the selected object becomes one borrowed fallback context for that
// source. Import never expands declarations or constructs forwarding edges.
class Import {
 public:
  constexpr Import(
      const Ttx::Concept::Documentation& documentation,
      TypeReference type_reference,
      Ttx::Lexical::Span span)
      : documentation(documentation),
        type_reference(type_reference),
        span(span) {}

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& {
    return documentation;
  }

  constexpr auto matches(const Import& other) const -> Bool {
    return type_reference.matches_route(other.type_reference);
  }

  constexpr auto get_type_reference() const -> const TypeReference& {
    return type_reference;
  }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return span; }

 private:
  const Ttx::Concept::Documentation& documentation;
  TypeReference type_reference;
  Ttx::Lexical::Span span;
};

}  // namespace Tetrodotoxin::Library::Language
