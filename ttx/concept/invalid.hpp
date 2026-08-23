// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Invalid is the closed unit type for a conceptual failure in the TTX graph.
// It is not a diagnostic, nullable stand in, partially constructed semantic
// object, or a subtype of every contract that might fail.
//
// All queries of any form on Invalid loops back to Invalid. This means any
// query that returns Invalid can be safely chained.
//
// Both identity and context resolution return the
// same Invalid. This preserves the first failed boundary and prevents secondary
// queries from manufacturing unrelated semantic results. Invalid is stateless
// and should gain no further concepts unless the fundamental Abstract contract
// changes.
class Invalid : public Abstract {
 public:
  TTX_CONTRACT(Invalid, Abstract);

  // Invalid has no object specific state. Every semantic failure returns this
  // one binary wide object so owners never store or construct failure state.
  static auto get_invalid() -> const Invalid&;

  Invalid(const Invalid&) = delete;
  auto operator=(const Invalid&) -> Invalid& = delete;

  TTX_NAME("Invalid"_view);

  TTX_EMPTY_DOCUMENTATION();

  constexpr auto resolve() const -> const Abstract& override { return *this; }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return *this;
  }

 private:
  constexpr Invalid() = default;
};

}  // namespace Ttx::Concept

// Abstracts with no contextual surface resolve every route to Invalid while
// preserving whether their virtual slot is constexpr.
#define TTX_CONSTEXPR_INVALID_CONTEXT                                 \
  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const \
      -> const Ttx::Concept::Abstract& override {                     \
    return Ttx::Concept::Invalid::get_invalid();                      \
  }

#define TTX_INVALID_CONTEXT                                 \
  auto resolve_context(Perimortem::Core::View::Bytes) const \
      -> const Ttx::Concept::Abstract& override {           \
    return Ttx::Concept::Invalid::get_invalid();            \
  }
