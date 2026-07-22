// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/constants/aggregate.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;

Tetrodotoxin::Model::Constants::Aggregate::Aggregate(
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Model::Type& type,
    View::Vector<Ttx::Concept::Reference<Ttx::Model::Constant>> values)
    : type(type), values(arena) {
  this->values.reset(values.get_size());
  for (Count i = 0; i < values.get_size(); i++) {
    this->values.insert(values[i]);
  }
}

auto Tetrodotoxin::Model::Constants::Aggregate::equals(
    const Ttx::Model::Constant& rhs) const -> Bool {
  if (!rhs.is<Aggregate>() || !has_same_type(rhs)) {
    return False;
  }
  const Aggregate& other = rhs.assume<Aggregate>();
  if (get_size() != other.get_size()) {
    return False;
  }
  for (Count i = 0; i < get_size(); i++) {
    if (get_value(i).assume<Ttx::Model::Constant>() !=
        other.get_value(i).assume<Ttx::Model::Constant>()) {
      return False;
    }
  }
  return True;
}

auto Tetrodotoxin::Model::Constants::Aggregate::get_value(Count index) const
    -> const Ttx::Concept::Abstract& {
  if (index >= values.get_size()) {
    return Ttx::Concept::Invalid::get_invalid();
  }
  return values.get_view()[index].get();
}
