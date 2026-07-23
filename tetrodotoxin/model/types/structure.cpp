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

auto Tetrodotoxin::Model::Types::Structure::resolve_root(
    View::Bytes route) const -> const Abstract& {
  return members.resolve_root(route);
}

auto Tetrodotoxin::Model::Types::Structure::resolve_static(
    View::Bytes route) const -> const Abstract& {
  return members.resolve_static(route);
}

auto Tetrodotoxin::Model::Types::Structure::resolve_exported_static(
    View::Bytes route) const -> const Abstract& {
  return completed ? members.resolve_exported_static(route)
                   : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Types::Structure::resolve_self(
    View::Bytes route) const -> const Abstract& {
  return members.resolve_self(route);
}

auto Tetrodotoxin::Model::Types::Structure::resolve_exported_self(
    View::Bytes route) const -> const Abstract& {
  return completed ? members.resolve_exported_self(route)
                   : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Types::Structure::add_field(
    const Addressables::Field& field,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_root(field, outer_context);
  if (!added) {
    return False;
  }

  fields.insert(Reference<Ttx::Model::Addressable>(field));
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::add_exported_field(
    const Addressables::Field& field,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_export(field, outer_context);
  if (!added) {
    return False;
  }

  fields.insert(Reference<Ttx::Model::Addressable>(field));
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::add_exposed_field(
    const Addressables::Field& field,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_exposed(field, outer_context);
  if (!added) {
    return False;
  }

  fields.insert(Reference<Ttx::Model::Addressable>(field));
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::add_member(
    const Abstract& member,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_root(member, outer_context);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::add_exported_member(
    const Abstract& member,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_export(member, outer_context);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::add_static(
    const Ttx::Model::Callables::Static& callable,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_static(callable, outer_context);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::add_exported_static(
    const Ttx::Model::Callables::Static& callable,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_exported_static(callable, outer_context);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::add_self(
    const Ttx::Model::Callables::Self& callable,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_self(callable, outer_context);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::add_exported_self(
    const Ttx::Model::Callables::Self& callable,
    const Abstract& outer_context) -> Bool {
  if (completed) {
    return False;
  }

  Bool added = members.add_exported_self(callable, outer_context);
  return added;
}

auto Tetrodotoxin::Model::Types::Structure::complete() -> Bool {
  if (completed) {
    return False;
  }

  Bool sealed = members.seal();
  if (!sealed) {
    return False;
  }

  layout = Ttx::Model::Layouts::Structured(fields.get_view());
  completed = True;
  return True;
}

auto Tetrodotoxin::Model::Types::Structure::set_shader_type(
    const Ttx::Model::Type& type) -> Bool {
  if (completed || represented || &type.resolve() == &Invalid::get_invalid()) {
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

auto Tetrodotoxin::Model::Types::Structure::get_export(Count index) const
    -> const Abstract& {
  return members.get_export(index);
}
