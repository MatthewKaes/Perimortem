// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/shaders/binary.hpp"

#include "tetrodotoxin/model/types/represented.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/vector.hpp"

using namespace Ttx::Concept;

static auto shader_type(const Ttx::Model::Type& source)
    -> const Ttx::Model::Type& {
  if (source.is<Tetrodotoxin::Model::Types::Represented>()) {
    return source.assume<Tetrodotoxin::Model::Types::Represented>()
        .get_shader_type();
  }

  return source;
}

static auto is_floating(const Ttx::Model::Type& source) -> Bool {
  const Ttx::Model::Type& type = shader_type(source);
  if (type.is<Ttx::Model::Types::Real>()) {
    return True;
  }

  return type.is<Ttx::Model::Types::Vector>() &&
         type.assume<Ttx::Model::Types::Vector>()
             .get_element_type()
             .resolve()
             .is<Ttx::Model::Types::Real>();
}

auto Tetrodotoxin::Model::Shaders::Binary::resolve(
    Tetrodotoxin::Model::Operations::Binary::Operator operation,
    const Ttx::Model::Type& left,
    const Ttx::Model::Type& right) -> const Abstract& {
  switch (operation) {
  case Tetrodotoxin::Model::Operations::Binary::Operator::Add:
  case Tetrodotoxin::Model::Operations::Binary::Operator::Subtract:
  case Tetrodotoxin::Model::Operations::Binary::Operator::Multiply:
  case Tetrodotoxin::Model::Operations::Binary::Operator::Divide:
    if (&left.resolve() == &right.resolve() && is_floating(left)) {
      return left;
    }
    return Invalid::get_invalid();
  }

  return Invalid::get_invalid();
}
