// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Invalid is the stable answer to a semantic query whose requested meaning is
// unavailable. A partially constructed identity can still remain in the graph
// and resolve to Invalid until its owner establishes that answer. The identity
// is not replaced by this object, which keeps tooling able to observe every
// other fact the source transaction already produced.
//
// Invalid carries no diagnostic because the owner with source context explains
// why the query failed. Identity and context resolution return the same binary
// wide object, so a later query preserves the original failed boundary instead
// of manufacturing a nullable edge or an unrelated semantic result.
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
