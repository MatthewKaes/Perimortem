// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/source.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Source::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Abstract::implements(requested);
}

auto Tetrodotoxin::Model::Source::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Tetrodotoxin::Model::Source::evaluate(
    const Dialect& dialect,
    Ttx::Lexical::Cursor& cursor) -> const Abstract& {
  // The Dialect produces a complete result before Source mutates its graph.
  // Invalid therefore leaves no partial root or Dialect edge behind.
  const Abstract& result = dialect.evaluate(cursor, *this);
  if (result.is<Invalid>()) {
    return Invalid::get_invalid();
  }

  Bool rooted = add_root(result, dialect);
  if (!rooted) {
    cursor.create_error(
        "The Dialect result conflicts with an existing Source root."_view);
    return Invalid::get_invalid();
  }

  return result;
}

auto Tetrodotoxin::Model::Source::add_root(
    const Abstract& definition,
    const Dialect& dialect) -> Bool {
  View::Bytes name = definition.get_name();

  // Identity protects anonymous roots while contextual lookup protects names
  // already owned by either a definition or Dependency Alias.
  for (Count i = 0; i < roots.get_size(); i++) {
    if (&roots[i].get_definition() == &definition) {
      return False;
    }
  }

  if (!name.is_empty() && !resolve_context(name).is<Invalid>()) {
    return False;
  }

  roots.insert(Root(definition, dialect));
  if (!name.is_empty()) {
    definitions_by_name.insert(name, Reference<Abstract>(definition));
  }

  return True;
}

auto Tetrodotoxin::Model::Source::depend(const Dependency& dependency) -> Bool {
  const Ttx::Model::Alias& binding = dependency.get_binding();
  const View::Bytes name = binding.get_name();

  // Only the Alias enters Source lookup. The Dependency remains an ordered
  // locator edge used by formatters, caches, and package closure collection.
  if (name.is_empty() || !resolve_context(name).is<Invalid>()) {
    return False;
  }

  for (Count i = 0; i < dependencies.get_size(); i++) {
    if (&dependencies[i].get() == &dependency) {
      return False;
    }
  }

  dependencies.insert(Reference<Dependency>(dependency));
  dependencies_by_name.insert(name, Reference<Ttx::Model::Alias>(binding));
  return True;
}

auto Tetrodotoxin::Model::Source::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Definitions::Entry* selected = definitions_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  const Dependencies::Entry* dependency = dependencies_by_name.find(route);
  if (dependency != nullptr) {
    return dependency->value.get();
  }

  return Invalid::get_invalid();
}
