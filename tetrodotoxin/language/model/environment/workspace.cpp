// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/environment/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Ttx::Model;

class Generics {
 public:
  enum class Slot : Unsigned_8 {
    Access,
    Fixed,
    View,
    Invalid,
  };

  // The table stores only stable slots. Formula objects remain owned by each
  // Environment so lookup identity and materialization lifetime share the same
  // transaction boundary.
  static constexpr Static::Vector<Pair<View::Bytes, Slot>, 3>
      generics_to_slots = {{
        {Ttx::Model::Types::Generics::Access::name, Slot::Access},
        {Ttx::Model::Types::Generics::Fixed::name, Slot::Fixed},
        {Ttx::Model::Types::Generics::View::name, Slot::View},
      }};
  using Table = Perimortem::Utility::Table<Slot, generics_to_slots>;
};

static_assert(
    Unsigned_8(Generics::Slot::Invalid) ==
    Generics::generics_to_slots.get_size());

auto Environment::Workspace::bind(
    View::Bytes local_name,
    const Abstract& target,
    const Documentation& documentation) -> Bool {
  if (local_name.is_empty() || target.is<Invalid>() ||
      !resolve_context(local_name).is<Invalid>()) {
    return False;
  }

  View::Bytes owned_name = arena.proxy(local_name);
  const auto& binding =
      arena.construct<Ttx::Model::Alias>(owned_name, target, documentation);
  bindings_by_name.insert(owned_name, Reference<Ttx::Model::Alias>(binding));
  return True;
}

auto Environment::Workspace::resolve_context(View::Bytes route) const
    -> const Abstract& {
  Generics::Slot formula =
      Generics::Table::find_or_default(route, Generics::Slot::Invalid);
  if (formula != Generics::Slot::Invalid) {
    return generic_formulas[Unsigned_8(formula)].get();
  }

  const Bindings::Entry* selected = bindings_by_name.find(route);
  if (selected != nullptr) {
    return selected->value.get();
  }

  return Invalid::get_invalid();
}
