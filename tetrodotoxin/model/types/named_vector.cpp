// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/types/named_vector.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Types::NamedVector::NamedVector(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Ttx::Model::Type& element_type,
    Count element_count)
    : name(arena.proxy(name)),
      element_type(element_type),
      members(arena),
      fields(arena) {
  constexpr Static::Vector<View::Bytes, 4> names = {{
    "x"_view,
    "y"_view,
    "z"_view,
    "w"_view,
  }};
  if (name.is_empty() || element_count == 0 ||
      element_count > names.get_size()) {
    __builtin_trap();
  }

  fields.reset(element_count);
  for (Count i = 0; i < element_count; i++) {
    const auto& field =
        arena.construct<Addressables::Field>(names[i], element_type);
    Bool added = members.add(field, True);
    if (!added) {
      __builtin_trap();
    }

    fields.insert(Reference<Ttx::Model::Addressable>(field));
  }
  layout = Ttx::Model::Layouts::Structured(fields.get_view());
}

auto Tetrodotoxin::Model::Types::NamedVector::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return members.resolve_context(route);
}

auto Tetrodotoxin::Model::Types::NamedVector::get_member(Count index) const
    -> const Ttx::Concept::Abstract& {
  return members.get_root(index);
}
