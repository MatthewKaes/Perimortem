// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/types/structure.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Types::Structure::Structure(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Documentation& documentation)
    : name(arena.proxy(name)),
      documentation(documentation),
      members(arena),
      fields(arena),
      shader_type(*this) {}

auto Tetrodotoxin::Model::Types::Structure::resolve() const -> const Abstract& {
  return completed ? static_cast<const Abstract&>(*this)
                   : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Types::Structure::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return completed ? members.resolve_context(route) : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Types::Structure::add_field(
    const Addressables::Field& field,
    Bool publish) -> Bool {
  if (completed) {
    return False;
  }
  Bool added = members.add(field, publish);
  if (!added) {
    return False;
  }

  fields.insert(Reference<Ttx::Model::Addressable>(field));
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::add_member(
    const Abstract& member,
    Bool publish) -> Bool {
  if (completed) {
    return False;
  }
  Bool added = members.add(member, publish);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::complete() -> Bool {
  if (completed) {
    return False;
  }
  layout = Ttx::Model::Layouts::Structured(fields.get_view());
  completed = True;
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::set_shader_type(
    const Ttx::Model::Type& type) -> Bool {
  if (represented || &type.resolve() == &Invalid::get_invalid()) {
    return False;
  }
  shader_type = Reference<Ttx::Model::Type>(type);
  represented = True;
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::get_member(Count index) const
    -> const Abstract& {
  return members.get_root(index);
}
