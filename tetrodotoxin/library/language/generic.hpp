// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Generic is the Types model's Abstract contract for a named compile time
// formula. It generates Types but is not itself a Type. The formula declares
// its complete ordered signature so the parser can validate arguments and own
// diagnostics before asking it for one stable materialized Type identity.
class Generic : public Ttx::Concept::Abstract {
 public:
  enum class Parameters : Unsigned_8 {
    Type,
    Unsigned_64,
    Signed_64,
    Bool,
  };

  // Semantic graph queries expose const references. Scalar arguments are
  // copied directly, while Type arguments retain their resolved identity.
  using Argument = Perimortem::Core::Static::
      Union<const Ttx::Model::Type&, ::Unsigned_64, ::Signed_64, ::Bool>;

  // Materializations is the append only writer for concrete Types generated
  // during one graph construction transaction. Formula objects remain
  // immutable rules. The writer owns the resulting identities and retains the
  // complete formula and argument key needed by progressive passes.
  //
  // Type arguments and created Types must already resolve canonically to
  // themselves before the key can be published. Resolved formulas and Type
  // arguments must outlive the writer's last query for every retained
  // materialization that refers to them.
  class Materializations {
   public:
    Materializations(Perimortem::Memory::Allocator::Arena& arena)
        : arena(arena), entries(arena) {}

    auto materialize(
        const Generic& generic,
        Perimortem::Core::View::Vector<Argument> arguments)
        -> Perimortem::Utility::Option<const Ttx::Model::Type&>;

    auto get_size() const -> Count { return entries.get_size(); }

   private:
    class Key {
     public:
      constexpr Key(
          const Ttx::Concept::Abstract& formula,
          Perimortem::Core::View::Vector<Argument> arguments)
          : formula(formula), arguments(arguments) {}

      constexpr auto operator==(const Key& candidate) const -> Bool {
        if (&formula != &candidate.formula ||
            arguments.get_size() != candidate.arguments.get_size()) {
          return False;
        }

        for (Count i = 0; i < arguments.get_size(); i++) {
          if (arguments[i] != candidate.arguments[i]) {
            return False;
          }
        }

        return True;
      }

      auto hash() const -> Unsigned_64;

     private:
      const Ttx::Concept::Abstract& formula;
      Perimortem::Core::View::Vector<Argument> arguments;
    };

    // Active is transient call stack state. It rejects direct reentrancy and
    // longer same key cycles without publishing a failure key or introducing
    // a durable construction epoch into the semantic graph.
    class Active {
     public:
      constexpr Active(const Key& key, Active* previous)
          : key(key), previous(previous) {}

      Key key;
      Active* previous;
      Bool reentered = False;
    };

    Perimortem::Memory::Allocator::Arena& arena;
    Perimortem::Memory::Managed::Map<Key, Ttx::Model::Type&> entries;
    Active* active = nullptr;
  };

  using ClassCatagory = Generic;
  static constexpr Perimortem::System::Uuid contract_id{
    0x8fe47e7b2c394bd7,
    0x9b3824546cc3bb50,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Concept::Abstract::implements(requested);
  }

  virtual constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> = 0;

 protected:
  // None means the supplied values do not satisfy this formula. Construction
  // occurs only after Materializations has validated the complete resolved
  // identity key and missed an existing result. Returning an incomplete or
  // redirected Type is rejection. The parser retains the source tokens and
  // owns the resulting diagnostic.
  virtual auto create(
      Perimortem::Core::View::Vector<Argument> arguments,
      Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> = 0;
};

}  // namespace Tetrodotoxin::Library::Language
