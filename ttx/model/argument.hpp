// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"

#include "ttx/concept/reference.hpp"

namespace Ttx::Model {

// Argument is one fully resolved input to a Generic formula. Abstract inputs
// are normalized through resolve() when the Argument is constructed. Aliases
// and explicit expression redirections can therefore share one materialized
// Type when they represent the same final semantic object. Constants resolve
// to themselves so their values remain part of the cache key.
//
// Bool and unsigned values are the closed inline scalar set for this slice.
// Other compile-time facts remain extensible as Abstract contracts. The
// underlying Union is never null in a constructed Argument. A null Union from
// internal construction normalizes to Invalid rather than trapping.
class Argument {
 public:
  using Value = Perimortem::Core::Static::
      Union<Concept::Reference<Concept::Abstract>, Bool, Unsigned_64>;

  template <typename Candidate>
    requires(__is_constructible(Value, Candidate &&))
  Argument(Candidate&& candidate)
      : value(normalize(Value(static_cast<Candidate&&>(candidate)))) {}

  constexpr auto get_value() const -> const Value& { return value; }

  auto operator==(const Argument& rhs) const -> Bool;
  auto operator!=(const Argument& rhs) const -> Bool;

 private:
  static auto normalize(Value value) -> Value;

  Value value;
};

}  // namespace Ttx::Model
