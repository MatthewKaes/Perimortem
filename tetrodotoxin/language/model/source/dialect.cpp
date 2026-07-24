// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/source/dialect.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Ttx::Model;

auto Source::Dialect::implements(Perimortem::System::Uuid requested) const
    -> Bool {
  return requested == contract_id || Abstract::implements(requested);
}

auto Source::Dialect::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Source::Dialect::resolve_context(View::Bytes) const -> const Abstract& {
  return Invalid::get_invalid();
}
