// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/source/code.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Model;

auto Source::Code::implements(Perimortem::System::Uuid requested) const
    -> Bool {
  return requested == contract_id || Abstract::implements(requested);
}

auto Source::Code::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Source::Code::evaluate(
    const Dialect& dialect,
    Ttx::Lexical::Errors& errors) -> const Abstract& {
  Ttx::Lexical::Cursor cursor(tokenizer, errors);

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

auto Source::Code::add_root(const Abstract& definition, const Dialect& dialect)
    -> Bool {
  View::Bytes name = definition.get_name();

  // Identity protects anonymous roots while contextual lookup protects names
  // already owned by either a definition or Environment binding.
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

auto Source::Code::resolve_context(View::Bytes route) const -> const Abstract& {
  const Definitions::Entry* selected = definitions_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  return environment.resolve_context(route);
}
