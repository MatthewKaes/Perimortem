// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/implementation.hpp"

#include "tetrodotoxin/library/language/constants/implementation.hpp"
#include "tetrodotoxin/library/language/types/interface.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

auto Types::Implementation::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Implementation::create_empty(arena, *this);
}

auto Types::Implementation::accepts(const Model::Pack& source) const -> Bool {
  BAIL_IF(source.get_layout().get_size() != 1);
  const Abstract& candidate = source.get_value_type(0).resolve();
  auto object = candidate.select<Types::Object>();
  return object &&
         (object->satisfies(requirement.get()) ||
          object->get_definition().get_host().satisfies(requirement.get()));
}

auto Types::Implementation::validate_layout(Ttx::Lexical::Cursor& cursor) const
    -> Bool {
  if (&requirement.get().resolve() == &requirement.get()) {
    return True;
  }

  cursor.create_error(
      "Implementation requires one completed semantic Type."_view,
      "Complete the selected requirement before using its erased value."_view);
  return False;
}

auto Types::Implementation::resolve_type_access(
    const Abstract& host,
    View::Bytes route,
    Model::Type::Access access) const -> const Abstract& {
  auto interface = requirement.get().resolve().select<Types::Interface>();
  if (!interface || access != Model::Type::Access::Self) {
    return Invalid::get_invalid();
  }

  return interface->resolve_type_access(host, route, access);
}

auto Types::Implementation::resolve_type_call(
    const Abstract& host,
    View::Bytes route,
    Model::Type::Access access) const -> const Abstract& {
  auto interface = requirement.get().resolve().select<Types::Interface>();
  if (!interface || access != Model::Type::Access::Self) {
    return Invalid::get_invalid();
  }

  return interface->resolve_type_call(host, route, access);
}
