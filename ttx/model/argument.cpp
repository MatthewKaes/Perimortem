// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/argument.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/constant.hpp"

auto Ttx::Model::Argument::operator==(const Argument& rhs) const -> Bool {
  if (value == rhs.value) {
    return True;
  }

  return value.visit(
      []() -> Bool { return False; },
      [&rhs](const Concept::Reference<Concept::Abstract>& lhs) -> Bool {
        return rhs.value.visit(
            []() -> Bool { return False; },
            [&lhs](const Concept::Reference<Concept::Abstract>& candidate)
                -> Bool {
              const Concept::Abstract& lhs_abstract = lhs.get();
              const Concept::Abstract& rhs_abstract = candidate.get();
              return lhs_abstract.is<Constant>() &&
                     rhs_abstract.is<Constant>() &&
                     lhs_abstract.as<Constant>() == rhs_abstract.as<Constant>();
            },
            [](auto) -> Bool { return False; });
      },
      [](auto) -> Bool { return False; });
}

auto Ttx::Model::Argument::operator!=(const Argument& rhs) const -> Bool {
  return !(*this == rhs);
}

auto Ttx::Model::Argument::normalize(Value value) -> Value {
  return value.visit(
      []() -> Value {
        static const Concept::Invalid invalid;
        return Concept::Reference<Concept::Abstract>(invalid);
      },
      [](const Concept::Reference<Concept::Abstract>& abstract) -> Value {
        return Concept::Reference<Concept::Abstract>(abstract.get().resolve());
      },
      [](Bool flag) -> Value { return Value(flag); },
      [](Bits_64 unsigned_value) -> Value { return Value(unsigned_value); });
}
