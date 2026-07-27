// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/uuid.hpp"

#include "ttx/concept/documentation.hpp"

namespace Ttx::Concept {

// Abstract is the root of the open TTX semantic graph. Narrower contracts
// extend its queries through public inheritance without teaching a central
// registry every possible object kind.
//
// The lexer deliberately provides useful name and token distinctions without
// semantic feedback. Resolution can therefore proceed inside the most local
// context available. Additional context may refine an incomplete
// query, but it must not make an earlier correct partial query incorrect. TTX
// builds its graph by stacking these contexts instead of consulting one global
// authority.
//
// Graph construction consequently enriches the same semantic objects instead
// of constructing parallel path or schema representations. Useful hierarchy
// emerges from the virtual contracts implemented by each Abstract.
//
// A route is only the borrowed source bytes that remain to be resolved. The
// queried Abstract owns the grammar, lookup structure, and slicing appropriate
// to its context. This keeps the base independent of allocation and global
// state. The surface is restricted rather than closed: additions must be
// fundamental to every semantic object, not conveniences for one derived
// contract.
class Abstract {
 public:
  using ClassCatagory = Abstract;
  static constexpr Perimortem::System::Uuid contract_id{
    0x67e0e29bc31340ef,
    0xb8fae3b06a1a7be5,
  };

  constexpr virtual ~Abstract() = default;

  // Proves a semantic contract without C++ RTTI or a central class registry.
  // Derived contracts recognize their stable identifier and then delegate to
  // their base contract. These identifiers describe interfaces only. Object
  // identity and durable names continue to come from the Abstract graph. A
  // native implementation may return true only for public C++ base contracts,
  // each represented by one unique accessible base subobject. This invariant
  // makes visitor dispatch well defined.
  virtual constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool {
    return requested == contract_id;
  }

  template <typename Requested>
  constexpr auto is() const -> Bool {
    static_assert(
        __is_base_of(Abstract, Requested),
        "A requested TTX contract must derive from Abstract.");
    static_assert(
        __is_same(Requested, typename Requested::ClassCatagory),
        "Only declared TTX contracts can be queried.");
    return implements(Requested::contract_id);
  }

  // Dispatches one proven public contract without exposing an unchecked
  // narrowed reference. A successful match receives the real Requested
  // object. A mismatch receives this exact Abstract so the caller can preserve
  // identity, report context, or continue through another query.
  //
  // The callbacks own the result of the operation. visit() only selects which
  // callback runs and forwards that callback's result.
  template <typename Requested, typename MatchVisitor, typename MismatchVisitor>
  constexpr auto visit(
      MatchVisitor match_visitor,
      MismatchVisitor mismatch_visitor) const -> decltype(auto) {
    if (is<Requested>()) {
      return match_visitor(static_cast<const Requested&>(*this));
    }

    return mismatch_visitor(*this);
  }

  // Gets the name of this Abstract.
  // If a canonical name is required then first call `resolve()`:
  // `canonical_name = abstract.resolve().get_name();`
  virtual constexpr auto get_name() const -> Perimortem::Core::View::Bytes = 0;

  // Returns the Abstract represented by this name. Alias uses this query to
  // redirect identity while ordinary Abstracts return themselves or an Abstract
  // that represents the intended canonical identity.
  //
  // For an unchanged valid DAG, resolving is idempotent:
  // `&abstract.resolve() == &abstract.resolve().resolve()`.
  virtual constexpr auto resolve() const -> const Abstract& { return *this; }

  // Resolves the borrowed route inside this Abstract's context. Implementations
  // decide how much of the route to consume and which Abstract receives the
  // remaining suffix if any.
  //
  // An Abstract can also completely change the context, but typically a resolve
  // in a context should consume the entire route or forward a slice.
  //
  // Abstracts might optimize route resolution, so this:
  // `abstract.resolve_context("Name").resolve_context("Name2");`
  // might return the same resulting Abstract as this:
  // `abstract.resolve_context("Name::Name2");`
  //
  // Neither partitioning is required to be optimized or equivalent. An empty
  // route is not required to behave like `resolve()`. A context may forward an
  // unchanged route while changing context, but valid DAG construction must
  // still guarantee that resolution terminates.
  //
  // For an unchanged DAG, repeating the same ordered resolution chain from the
  // same starting Abstract returns the same final Abstract identity.
  virtual constexpr auto resolve_context(
      Perimortem::Core::View::Bytes route) const -> const Abstract& = 0;

  // Returns the documentation visible at this exact Abstract. The concrete
  // object may own authored prose, expose a generated comment, forward another
  // object's documentation, or compose several sources. This query does not
  // resolve identity implicitly.
  //
  // The returned object and every borrowed line remain valid for the lifetime
  // of this Abstract. Missing documentation is represented by an empty
  // Documentation object, never Invalid or a nullable reference.
  virtual constexpr auto get_documentation() const
      -> const Concept::Documentation& = 0;
};

}  // namespace Ttx::Concept
