// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Materializations owns the append only Types generated during one graph
// construction transaction. Formula objects remain immutable rules while this
// owner retains the identities and complete keys needed by progressive passes.
//
// Direct Type arguments are stable identities and may still be completing
// their owner-defined Layout. Formula identities and Type arguments must
// outlive the final query for every retained materialization.
class Materializations {
 public:
  Materializations(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena), entries(arena) {}

  auto materialize(
      const Generic& generic,
      Perimortem::Core::View::Vector<Generic::Argument> arguments)
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  // Authored Generic application supplies one real linked Layout. This owner
  // fits its entries against the formula's ordered parameter contract and
  // normalizes literal identities into the semantic values used by the one
  // canonical materialization key.
  auto materialize(
      const Generic& generic,
      const Ttx::Concept::Layout& arguments)
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto get_size() const -> Count { return entries.get_size(); }

  auto find_invalid_value_type() const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

 private:
  class Key {
   public:
    constexpr Key(
        const Ttx::Concept::Abstract& formula,
        Perimortem::Core::View::Vector<Generic::Argument> arguments)
        : formula(formula), arguments(arguments) {}

    constexpr auto operator==(const Key& candidate) const -> Bool {
      if (&formula != &candidate.formula ||
          arguments.get_size() != candidate.arguments.get_size()) {
        return False;
      }

      const auto* arguments_data = arguments.get_data();
      const auto* candidate_data = candidate.arguments.get_data();
      for (Count i = 0; i < arguments.get_size(); i++) {
        if (arguments_data[i] != candidate_data[i]) {
          return False;
        }
      }

      return True;
    }

    auto hash() const -> Unsigned_64;

   private:
    const Ttx::Concept::Abstract& formula;
    Perimortem::Core::View::Vector<Generic::Argument> arguments;
  };

  // Active is call stack state. It rejects repeated keys without publishing a
  // failure key or adding a durable construction epoch to the semantic graph.
  class Active {
   public:
    constexpr Active(const Key& key, Active* previous)
        : key(key), previous(previous) {}

    Key key;
    Active* previous;
    Bool reentered = False;
  };

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Managed::
      Map<Key, Ttx::Concept::Reference<const Ttx::Model::Type>>
          entries;
  Active* active = nullptr;
};

}  // namespace Tetrodotoxin::Library::Language
