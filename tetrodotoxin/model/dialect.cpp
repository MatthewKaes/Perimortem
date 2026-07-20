// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/dialect.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Dialect::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Abstract::implements(requested);
}

auto Tetrodotoxin::Model::Dialect::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Tetrodotoxin::Model::Dialect::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}
