// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/group.hpp"

#include "ttx/concept/invalid.hpp"

Ttx::Model::Group::Group(
    Perimortem::Core::View::Bytes name,
    Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
        abstracts,
    Concept::Documentation documentation)
    : name(name), abstracts(abstracts), documentation(documentation) {}

auto Ttx::Model::Group::implements(Perimortem::System::Uuid requested) const
    -> Bool {
  return requested == contract_id || Abstract::implements(requested);
}

auto Ttx::Model::Group::get_name() const -> Perimortem::Core::View::Bytes {
  return name;
}

auto Ttx::Model::Group::get_abstracts() const
    -> Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>> {
  return abstracts;
}

auto Ttx::Model::Group::get_documentation() const -> Concept::Documentation {
  return documentation;
}

auto Ttx::Model::Group::resolve_context(
    Perimortem::Core::View::Bytes route) const -> const Concept::Abstract& {
  Count matches = 0;
  Count selected = 0;
  for (Count i = 0; i < abstracts.get_size(); i++) {
    if (abstracts[i].get().get_name() == route) {
      selected = i;
      matches++;
    }
  }

  if (matches != 1) {
    return Concept::Invalid::get_invalid();
  }

  return abstracts[selected].get();
}
