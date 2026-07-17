// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/scope.hpp"

#include "ttx/concept/invalid.hpp"

Ttx::Model::Scope::Scope(
    const Concept::Abstract& local,
    const Concept::Abstract& outer)
    : local(local), outer(outer) {}

auto Ttx::Model::Scope::implements(Perimortem::System::Uuid requested) const
    -> Bool {
  return requested == contract_id || Abstract::implements(requested);
}

auto Ttx::Model::Scope::get_name() const -> Perimortem::Core::View::Bytes {
  return local.get_name();
}

auto Ttx::Model::Scope::resolve_context(
    Perimortem::Core::View::Bytes route) const -> const Concept::Abstract& {
  const Concept::Abstract& selected = local.resolve_context(route);
  if (!selected.is<Concept::Invalid>()) {
    return selected;
  }

  return outer.resolve_context(route);
}
