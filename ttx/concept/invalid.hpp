// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Invalid is the closed unit type for a conceptual failure in the TTX graph.
// It is not a diagnostic, nullable stand-in, partially constructed semantic
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
  using ClassCatagory = Invalid;
  static constexpr Perimortem::System::Uuid contract_id{
    0x61aaefe33f534a6b,
    0x8c39f04bd4a2f525,
  };

  // Invalid has no object-specific state. Every semantic failure returns this
  // one binary-wide object so owners never store or construct failure state.
  static auto get_invalid() -> const Invalid&;

  Invalid(const Invalid&) = delete;
  auto operator=(const Invalid&) -> Invalid& = delete;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Invalid"_view;
  }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  constexpr auto resolve() const -> const Abstract& override { return *this; }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return *this;
  }

 private:
  constexpr Invalid() = default;
};

}  // namespace Ttx::Concept
