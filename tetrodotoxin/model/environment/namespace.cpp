// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/environment/namespace.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Model;

auto Environment::Namespace::construct(
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

  Bool sealed = result.seal();
  if (!sealed) {
    return Invalid::get_invalid();
  }

  return result;
}

Environment::Namespace::Namespace(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Documentation& documentation)
    : name(name),
      roots(arena),
      exports(arena),
      documentation(documentation),
      roots_by_name(arena),
      exports_by_name(arena),
      statics_by_name(arena),
      exported_statics_by_name(arena) {}

auto Environment::Namespace::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Ttx::Model::Exports::implements(requested);
}

auto Environment::Namespace::get_name() const -> View::Bytes {
  return name;
}

auto Environment::Namespace::get_documentation() const -> const Documentation& {
  return documentation;
}

auto Environment::Namespace::get_export_count() const -> Count {
  return exports.get_size();
}

auto Environment::Namespace::get_export(Count index) const -> const Abstract& {
  if (index >= exports.get_size()) {
    return Invalid::get_invalid();
  }

  return exports.at(index).get();
}

auto Environment::Namespace::get_root_count() const -> Count {
  return roots.get_size();
}

auto Environment::Namespace::get_root(Count index) const -> const Abstract& {
  if (index >= roots.get_size()) {
    return Invalid::get_invalid();
  }

  return roots.at(index).get();
}

auto Environment::Namespace::outer_contains(
    const Abstract& outer_context,
    View::Bytes name) const -> Bool {
  return !outer_context.resolve_context(name).is<Invalid>();
}

auto Environment::Namespace::add_root(
    const Abstract& definition,
    const Abstract& outer_context) -> Bool {
  const View::Bytes definition_name = definition.get_name();
  if (sealed || definition_name.is_empty() ||
      definition.is<Ttx::Model::Callables::Static>() ||
      definition.is<Ttx::Model::Callables::Self>() ||
      roots_by_name.find(definition_name) != nullptr ||
      outer_contains(outer_context, definition_name)) {
    return False;
  }

  roots_by_name.insert(definition_name, Reference<Abstract>(definition));
  roots.insert(Reference<Abstract>(definition));
  return True;
}

auto Environment::Namespace::add_export(
    const Abstract& definition,
    const Abstract& outer_context) -> Bool {
  const View::Bytes definition_name = definition.get_name();
  if (sealed || definition_name.is_empty() ||
      definition.is<Ttx::Model::Callables::Static>() ||
      definition.is<Ttx::Model::Callables::Self>() ||
      exports_by_name.find(definition_name) != nullptr ||
      outer_contains(outer_context, definition_name)) {
    return False;
  }

  // Publication can follow a Definitions transaction or construct a complete
  // public Namespace from existing edges. Either path must preserve one rooted
  // identity for the exported name.
  const RootIndex::Entry* rooted = roots_by_name.find(definition_name);
  if (rooted == nullptr) {
    roots_by_name.insert(definition_name, Reference<Abstract>(definition));
    roots.insert(Reference<Abstract>(definition));
  } else if (&rooted->value.get() != &definition) {
    return False;
  }

  exports_by_name.insert(definition_name, Reference<Abstract>(definition));
  exports.insert(Reference<Abstract>(definition));
  return True;
}

auto Environment::Namespace::add_exposed(
    const Ttx::Model::Addressables::Writable& definition,
    const Abstract& outer_context) -> Bool {
  const Ttx::Model::Addressable& read_only = definition.get_read_only();
  const Abstract& definition_type = definition.get_type().resolve();
  const Abstract& read_only_type = read_only.get_type().resolve();
  const View::Bytes definition_name = definition.get_name();
  if (sealed || definition_name.is_empty() ||
      read_only.get_name() != definition_name ||
      &read_only == static_cast<const Ttx::Model::Addressable*>(&definition) ||
      read_only.is<Ttx::Model::Addressables::Writable>() ||
      definition_type.is<Invalid>() || &read_only_type != &definition_type ||
      exports_by_name.find(definition_name) != nullptr ||
      outer_contains(outer_context, definition_name)) {
    return False;
  }

  const RootIndex::Entry* rooted = roots_by_name.find(definition_name);
  if (rooted != nullptr && &rooted->value.get() != &definition) {
    return False;
  }

  if (rooted == nullptr) {
    roots_by_name.insert(definition_name, Reference<Abstract>(definition));
    roots.insert(Reference<Abstract>(definition));
  }

  exports_by_name.insert(definition_name, Reference<Abstract>(read_only));
  exports.insert(Reference<Abstract>(read_only));
  return True;
}

auto Environment::Namespace::add_static(
    const Ttx::Model::Callables::Static& callable,
    const Abstract& outer_context) -> Bool {
  View::Bytes callable_name = callable.get_name();
  if (sealed || callable_name.is_empty() ||
      roots_by_name.find(callable_name) != nullptr ||
      statics_by_name.find(callable_name) != nullptr ||
      outer_contains(outer_context, callable_name)) {
    return False;
  }

  roots_by_name.insert(callable_name, Reference<Abstract>(callable));
  statics_by_name.insert(callable_name, Reference<Abstract>(callable));
  roots.insert(Reference<Abstract>(callable));
  return True;
}

auto Environment::Namespace::add_exported_static(
    const Ttx::Model::Callables::Static& callable,
    const Abstract& outer_context) -> Bool {
  View::Bytes callable_name = callable.get_name();
  if (sealed || callable_name.is_empty() ||
      exports_by_name.find(callable_name) != nullptr ||
      exported_statics_by_name.find(callable_name) != nullptr ||
      outer_contains(outer_context, callable_name)) {
    return False;
  }

  const RootIndex::Entry* rooted = roots_by_name.find(callable_name);
  const RootIndex::Entry* rooted_static = statics_by_name.find(callable_name);
  if ((rooted == nullptr) != (rooted_static == nullptr) ||
      (rooted != nullptr && &rooted->value.get() != &callable) ||
      (rooted_static != nullptr && &rooted_static->value.get() != &callable)) {
    return False;
  }

  if (rooted == nullptr) {
    roots_by_name.insert(callable_name, Reference<Abstract>(callable));
    statics_by_name.insert(callable_name, Reference<Abstract>(callable));
    roots.insert(Reference<Abstract>(callable));
  }

  exports_by_name.insert(callable_name, Reference<Abstract>(callable));
  exported_statics_by_name.insert(callable_name, Reference<Abstract>(callable));
  exports.insert(Reference<Abstract>(callable));
  return True;
}

auto Environment::Namespace::resolve_root(View::Bytes route) const
    -> const Abstract& {
  const RootIndex::Entry* selected = roots_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  return Invalid::get_invalid();
}

auto Environment::Namespace::resolve_static(View::Bytes route) const
    -> const Abstract& {
  const RootIndex::Entry* selected = statics_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  return Invalid::get_invalid();
}

auto Environment::Namespace::resolve_exported_static(View::Bytes route) const
    -> const Abstract& {
  const RootIndex::Entry* selected = exported_statics_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  return Invalid::get_invalid();
}

auto Environment::Namespace::resolve_context(View::Bytes route) const
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

auto Environment::Namespace::seal() -> Bool {
  if (sealed) {
    return False;
  }

  sealed = True;
  return True;
}
