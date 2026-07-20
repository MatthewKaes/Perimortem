// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/namespace.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Namespace::construct(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<Reference<Abstract>> exports,
    const Documentation& documentation) -> const Abstract& {
  Namespace& result = arena.construct<Namespace>(arena, name, documentation);
  for (Count i = 0; i < exports.get_size(); i++) {
    Bool published = result.add_export(exports[i].get());
    if (!published) {
      return Invalid::get_invalid();
    }
  }

  return result;
}

Tetrodotoxin::Model::Namespace::Namespace(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Documentation& documentation)
    : name(name),
      exports(arena),
      documentation(documentation),
      roots_by_name(arena),
      exports_by_name(arena) {}

auto Tetrodotoxin::Model::Namespace::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Ttx::Model::Exports::implements(requested);
}

auto Tetrodotoxin::Model::Namespace::get_name() const -> View::Bytes {
  return name;
}

auto Tetrodotoxin::Model::Namespace::get_documentation() const
    -> const Documentation& {
  return documentation;
}

auto Tetrodotoxin::Model::Namespace::get_export_count() const -> Count {
  return exports.get_size();
}

auto Tetrodotoxin::Model::Namespace::get_export(Count index) const
    -> const Abstract& {
  if (index >= exports.get_size()) {
    return Invalid::get_invalid();
  }

  return exports.at(index).get();
}

auto Tetrodotoxin::Model::Namespace::add_root(const Abstract& definition)
    -> Bool {
  const View::Bytes definition_name = definition.get_name();
  if (definition_name.is_empty() ||
      roots_by_name.find(definition_name) != nullptr) {
    return False;
  }

  roots_by_name.insert(definition_name, Reference<Abstract>(definition));
  return True;
}

auto Tetrodotoxin::Model::Namespace::add_export(const Abstract& definition)
    -> Bool {
  const View::Bytes definition_name = definition.get_name();
  if (definition_name.is_empty() ||
      exports_by_name.find(definition_name) != nullptr) {
    return False;
  }

  // Publication can follow a Definitions transaction or construct a complete
  // public Namespace from existing edges. Either path must preserve one rooted
  // identity for the exported name.
  const RootIndex::Entry* rooted = roots_by_name.find(definition_name);
  if (rooted == nullptr) {
    roots_by_name.insert(definition_name, Reference<Abstract>(definition));
  } else if (&rooted->value.get() != &definition) {
    return False;
  }

  exports_by_name.insert(definition_name, Reference<Abstract>(definition));
  exports.insert(Reference<Abstract>(definition));
  return True;
}

auto Tetrodotoxin::Model::Namespace::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // Exports is a closed visibility boundary. Retained roots are intentionally
  // absent from this lookup so an Alias cannot expose private producer state by
  // forwarding a name that was never published.
  const RootIndex::Entry* selected = exports_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  return Invalid::get_invalid();
}
