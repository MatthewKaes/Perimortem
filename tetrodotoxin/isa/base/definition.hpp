// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/base/expression/value.hpp"
#include "ttx/attribute.hpp"
#include "ttx/lexical/class.hpp"

namespace Tetrodotoxin::Isa::Base {

// Definition is immutable ISA enrichment attached to a stable TTX identity.
// Visibility, source attributes, and initializer syntax affect producers and
// lowerers, but none of them change a Member's structural type shape.
class Definition {
 public:
  constexpr Definition() = default;
  constexpr Definition(
      Ttx::Lexical::Class::Type modifier,
      Perimortem::Core::View::Vector<Ttx::Attribute> attributes = {},
      const Expression::Value* initializer = nullptr)
      : modifier(modifier), attributes(attributes), initializer(initializer) {}
  constexpr Definition(const Definition&) = default;
  constexpr auto operator=(const Definition& other) -> Definition& {
    modifier = other.modifier;
    attributes = Perimortem::Core::View::Vector<Ttx::Attribute>(
        other.attributes.get_data(), other.attributes.get_size());
    initializer = other.initializer;
    return *this;
  }

  constexpr auto get_modifier() const -> Ttx::Lexical::Class::Type {
    return modifier;
  }

  constexpr auto get_attributes() const
      -> Perimortem::Core::View::Vector<Ttx::Attribute> {
    return attributes;
  }

  constexpr auto get_initializer() const -> const Expression::Value* {
    return initializer;
  }

 private:
  Ttx::Lexical::Class::Type modifier = Ttx::Lexical::Class::Type::Unknown;
  Perimortem::Core::View::Vector<Ttx::Attribute> attributes;
  const Expression::Value* initializer = nullptr;
};

}  // namespace Tetrodotoxin::Isa::Base
