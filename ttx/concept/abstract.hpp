// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/concept/type_identity.hpp"

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
// A contextual query receives one borrowed, unqualified name. The concrete
// grammar operator owns punctuation and asks each selected Abstract about the
// next name. The queried Abstract owns only the lookup structure appropriate to
// its context. This keeps the base independent of allocation and global state.
// The surface is restricted rather than closed: additions must be fundamental
// to every semantic object, not conveniences for one derived contract.
class Abstract {
 public:
  using ClassCatagory = Abstract;

  constexpr virtual ~Abstract() = default;

  // Proves a semantic contract without C++ RTTI or a central class registry.
  // Derived contracts recognize their live type identity and then delegate to
  // their base contract. These identities describe interfaces only. Object
  // identity and durable names continue to come from the Abstract graph. A
  // native implementation may return true only for public C++ base contracts,
  // each represented by one unique accessible base subobject. This invariant
  // makes visitor dispatch well defined.
  virtual constexpr auto implements(::U64 requested) const -> Bool {
    return requested == get_type_identity<Abstract>();
  }

  template <typename Requested>
  constexpr auto is() const -> Bool {
    static_assert(
        __is_base_of(Abstract, Requested),
        "A requested TTX contract must derive from Abstract.");
    static_assert(
        __is_same(Requested, typename Requested::ClassCatagory),
        "Only declared TTX contracts can be queried.");
    return implements(get_type_identity<Requested>());
  }

  // Returns the proven contract as one borrowed reference. Absence preserves
  // the same mismatch result as is() without making every caller rebuild the
  // identical visit pair merely to retain the selected object.
  template <typename Requested>
  constexpr auto select() -> Perimortem::Core::Option<Requested&> {
    if (!is<Requested>()) {
      return {};
    }

    return static_cast<Requested&>(*this);
  }

  template <typename Requested>
  constexpr auto select() const -> Perimortem::Core::Option<const Requested&> {
    if (!is<Requested>()) {
      return {};
    }

    return static_cast<const Requested&>(*this);
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
      MismatchVisitor mismatch_visitor) -> decltype(auto) {
    if (is<Requested>()) {
      return match_visitor(static_cast<Requested&>(*this));
    }

    return mismatch_visitor(*this);
  }

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
  // `canonical_name = abstract.resolve().get_name()`
  virtual constexpr auto get_name() const -> Perimortem::Core::View::Bytes = 0;

  // Returns the Abstract represented by this name. Alias uses this query to
  // redirect identity while ordinary Abstracts return themselves or an Abstract
  // that represents the intended canonical identity.
  //
  // For an unchanged valid DAG, resolving is idempotent:
  // `&abstract.resolve() == &abstract.resolve().resolve()`.
  virtual constexpr auto resolve() const -> const Abstract& { return *this; }

  // Resolves one name inside this Abstract's context. Concrete language
  // operators own route punctuation and ask the selected Abstract about the
  // next name one step at a time. `Name::Name2` is therefore two ordered
  // queries, never one flattened map key or a request for Abstract to parse
  // another language's operator.
  //
  // An empty name is not required to behave like `resolve()`. A context may
  // forward an unchanged name while changing context, but valid DAG
  // construction must still guarantee that resolution terminates.
  //
  // For an unchanged DAG, repeating the same ordered resolution chain from the
  // same starting Abstract returns the same final Abstract identity.
  virtual constexpr auto resolve_context(
      Perimortem::Core::View::Bytes name) const -> const Abstract& = 0;

  // Resolves an Addressable or Callable selected by an explicit receiver.
  // These queries keep operator intent separate from lexical name resolution.
  // The selected Abstract and concrete language decide routing and authority.
  // TTX does not assign receiver roles, storage, visibility, or member policy.
  virtual auto resolve_access(
      const Abstract& host,
      Perimortem::Core::View::Bytes name) const -> const Abstract&;

  virtual auto resolve_call(
      const Abstract& host,
      Perimortem::Core::View::Bytes name) const -> const Abstract&;

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

// Keep each derived category declaration beside its direct semantic base while
// preserving the shared live proof implementation.
#define TTX_CONTRACT(type, base)                                      \
  using ClassCatagory = type;                                         \
  constexpr auto implements(::U64 requested) const -> Bool override { \
    return requested == Ttx::Concept::get_type_identity<type>() ||    \
           base::implements(requested);                               \
  }

// Compact exact implementations of Abstract's universal presentation slots.
#define TTX_NAME(expression)                                                  \
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override { \
    return expression;                                                        \
  }
