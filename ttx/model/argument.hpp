// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"

#include "ttx/abstraction/invalid.hpp"
#include "ttx/abstraction/reference.hpp"

namespace Ttx::Model {

// Argument is one fully resolved input to a Generic formula. Abstract inputs
// are normalized through resolve() when the Argument is constructed. Alias and
// expression spellings can therefore share one materialized Type when they
// represent the same final semantic object.
//
// Bool and unsigned values are the closed inline scalar set for this slice.
// Other compile-time facts remain extensible as Abstract contracts. The
// underlying Union is never null in a constructed Argument. A null Union from
// internal construction normalizes to Invalid rather than trapping.
class Argument {
 public:
  using Value = Perimortem::Core::Static::
      Union<Abstraction::Reference<Abstraction::Abstract>, Bool, Bits_64>;

  template <typename Candidate>
    requires(__is_constructible(Value, Candidate &&))
  Argument(Candidate&& candidate)
      : value(normalize(Value(static_cast<Candidate&&>(candidate)))) {}

  constexpr auto get_value() const -> const Value& { return value; }

  constexpr auto operator==(const Argument& rhs) const -> Bool {
    return value == rhs.value;
  }

  constexpr auto operator!=(const Argument& rhs) const -> Bool {
    return !(*this == rhs);
  }

 private:
  static auto normalize(Value value) -> Value {
    return value.visit(
        []() -> Value {
          static const Abstraction::Invalid invalid;
          return Abstraction::Reference<Abstraction::Abstract>(invalid);
        },
        [](const Abstraction::Reference<Abstraction::Abstract>& abstract)
            -> Value {
          return Abstraction::Reference<Abstraction::Abstract>(
              abstract.get().resolve());
        },
        [](Bool flag) -> Value { return Value(flag); },
        [](Bits_64 unsigned_value) -> Value { return Value(unsigned_value); });
  }

  Value value;
};

}  // namespace Ttx::Model
