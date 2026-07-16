// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/uuid.hpp"

namespace Ttx::Abstraction {

// TTX does not begin with a closed type system. It begins with named abstract
// contexts whose behavior can be extended by the language, an ISA, a compiler,
// or a host without teaching a central registry about every possible concept.
//
// The lexer deliberately provides useful name and token distinctions without
// parser feedback. The parser can therefore resolve progressively inside the
// most local context available. Additional context may refine an incomplete
// query, but it must not make an earlier correct partial query incorrect. TTX
// gets its type system by stacking these contexts instead of consulting one
// global type authority.
//
// Parsing and compiling consequently enrich the same semantic objects instead
// of constructing parallel path, schema, or compiler-owned representations.
// Types, aliases, packages, callables, reflection, and foreign-language
// contracts are Abstracts first. Their useful hierarchy emerges from the
// virtual contracts they implement.
//
// A route is only the borrowed source bytes that remain to be resolved. The
// queried Abstract owns the grammar, lookup structure, and slicing appropriate
// to its context. This keeps the base independent of allocation, global state,
// failure policy, package structure, and any particular runtime type system.
// The surface is restricted rather than closed: additions must be fundamental
// to every semantic object, not conveniences for one derived contract.
class Abstract {
 public:
  using ContractOwner = Abstract;
  static constexpr Perimortem::System::Uuid contract_id{
    0x67e0e29bc31340ef,
    0xb8fae3b06a1a7be5,
  };

  virtual ~Abstract() = default;

  // Proves a semantic contract without C++ RTTI or a central class registry.
  // Derived contracts recognize their stable identifier and then delegate to
  // their base contract. These identifiers describe interfaces only. Object
  // identity and durable names continue to come from the Abstract graph. A
  // native implementation may return true only for public C++ base contracts,
  // this invariant makes the checked reference conversion well-defined.
  virtual auto implements(Perimortem::System::Uuid requested) const -> Bool {
    return requested == contract_id;
  }

  template <typename Requested>
  auto is() const -> Bool {
    static_assert(
        __is_same(Requested, typename Requested::ContractOwner),
        "Only declared TTX contracts can be queried.");
    return implements(Requested::contract_id);
  }

  // Converts after proving the requested contract. A failed conversion is a
  // caller contract violation rather than a nullable semantic result; fallible
  // resolution returns Invalid before a narrow contract is requested.
  template <typename Requested>
  auto as() const -> const Requested& {
    if (!is<Requested>()) {
      __builtin_trap();
    }
    return static_cast<const Requested&>(*this);
  }

  // Gets the name of this Abstract.
  // If a canonical name is required then first call `resolve()`:
  // `canonical_name = abstract.resolve().get_name();`
  virtual auto get_name() const -> Perimortem::Core::View::Bytes = 0;

  // Returns the Abstract represented by this name. Alias uses this query to
  // redirect identity while ordinary Abstracts return themselves or an Abstract
  // that represents the intended canonical identity.
  //
  // For an unchanged valid DAG, resolving is idempotent:
  // `&abstract.resolve() == &abstract.resolve().resolve()`.
  virtual auto resolve() const -> const Abstract& { return *this; }

  // Resolves the borrowed route inside this Abstract's context. Implementations
  // decide how much of the route to consume and which Abstract receives the
  // remaining suffix if any.
  //
  // A resolver can also completely change the context, but typically a resolve
  // in a context should consume the entire route or forward a slice.
  //
  // Abstracts might optimize route resolution, so this:
  // `abstract.resolve_context("Name").resolve_context("Name2");`
  // might return the same resulting Abstract as this:
  // `abstract.resolve_context("Name::Name2");`
  //
  // Neither partitioning is required to be optimized or equivalent. An empty
  // route is not required to behave like `resolve()`. A resolver may forward an
  // unchanged route while changing context, but valid DAG construction must
  // still guarantee that resolution terminates.
  //
  // For an unchanged DAG, repeating the same ordered resolution chain from the
  // same starting Abstract returns the same final Abstract identity.
  virtual auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Abstract& = 0;
};

}  // namespace Ttx::Abstraction
