// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/types/generics/access.hpp"
#include "ttx/model/types/generics/fixed.hpp"
#include "ttx/model/types/generics/view.hpp"

namespace Tetrodotoxin::Ttx::Model::Environment {

// Workspace is the semantic environment supplied to every Source in one
// interpretation transaction. It owns local bindings, the common Generic
// formulas, and the append-only materializations shared by those Sources.
//
// Materialized
// Types therefore have one address across every member Source without relying
// on process static storage. A closed compile time name table selects the
// immutable formula objects while imported and host bindings remain in the
// separate dynamic map.
//
// Exact package resolution is intentionally absent until the resolved Package
// graph has a real Model owner. The parsed Package::Source descriptor is not a
// substitute for that graph.
class Workspace {
 public:
  Workspace()
      : materializations(arena),
        generic_formulas({access, fixed, view}),
        bindings_by_name(arena) {}

  auto bind(
      Perimortem::Core::View::Bytes local_name,
      const Ttx::Concept::Abstract& target,
      const Ttx::Concept::Documentation& documentation) -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_materializations()
      -> Ttx::Model::Types::Generic::Materializations& {
    return materializations;
  }

 private:
  using Bindings = Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Ttx::Model::Alias>>;
  using Formula = Ttx::Concept::Reference<Ttx::Concept::Abstract>;
  using Formulas = Perimortem::Core::Static::Vector<Formula, 3>;

  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Model::Types::Generics::View view;
  Ttx::Model::Types::Generics::Access access;
  Ttx::Model::Types::Generics::Fixed fixed;
  Ttx::Model::Types::Generic::Materializations materializations;
  // Slot order is Access, Fixed, View and matches the closed lookup table.
  Formulas generic_formulas;
  Bindings bindings_by_name;
};

}  // namespace Tetrodotoxin::Ttx::Model::Environment
