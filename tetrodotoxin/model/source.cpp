// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/source.hpp"

Tetrodotoxin::Model::Source::Source(
    Perimortem::Core::View::Bytes name,
    Perimortem::Core::View::Vector<
        Ttx::Concept::Reference<Ttx::Concept::Abstract>> definitions,
    const Ttx::Concept::Documentation& documentation)
    : definitions(name, definitions, documentation) {}

auto Tetrodotoxin::Model::Source::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id ||
         Ttx::Concept::Abstract::implements(requested);
}

auto Tetrodotoxin::Model::Source::get_name() const
    -> Perimortem::Core::View::Bytes {
  return definitions.get_name();
}

auto Tetrodotoxin::Model::Source::get_definitions() const
    -> const Ttx::Model::Group& {
  return definitions;
}

auto Tetrodotoxin::Model::Source::get_documentation() const
    -> const Ttx::Concept::Documentation& {
  return definitions.get_documentation();
}

auto Tetrodotoxin::Model::Source::resolve_context(
    Perimortem::Core::View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return definitions.resolve_context(route);
}
