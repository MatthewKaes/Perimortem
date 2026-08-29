// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/concept/type_identity.hpp"

namespace Ttx::Concept {

// Abstract gives every shared semantic object one stable identity and a small
// set of questions that any tool can ask. Narrower contracts add Type,
// SemanticPack, Addressable, Callable, and Alias behavior through ordinary
// inheritance, which lets the concrete language remain the owner of each
// object.
//
// QueryContext lookup follows the language route one name at a time. The
// selected object answers from the context it actually owns, so Packages and
// Dialects can compose without a global member registry.
//
// Construction enriches these same objects as more context becomes available.
// An identity that has already answered successfully stays stable, giving
// editors, compilers, and runtimes one graph to share throughout completion.
class Abstract {
 public:
  // SemanticLayout is the identity-free shape of semantic flow. It borrows the
  // exact Abstracts that supply each slot, so ordering and fitting never create
  // a second semantic graph.
  class SemanticLayout {
   public:
    enum class Errors : U8 {
      IndexOutOfBounds,
      SizeMismatch,
      IncompatibleFit,
    };

    constexpr virtual ~SemanticLayout() = default;

    virtual constexpr auto get_size() const -> Count = 0;
    virtual constexpr auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Abstract&> = 0;
    virtual constexpr auto get_name(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
      return {};
    }
    virtual constexpr auto fits_entry(
        const SemanticLayout& target,
        Count source_index,
        Count target_index) const -> Bool = 0;
    constexpr auto fits(const SemanticLayout& target) const -> Bool {
      return get_size() == target.get_size() && fits_at(target, 0);
    }
    virtual constexpr auto fits_at(
        const SemanticLayout& target,
        Count target_offset) const -> Bool = 0;
    constexpr auto get_fitted(const SemanticLayout& target, Count target_index)
        const -> Perimortem::Utility::Result<const Abstract&, Errors> {
      if (target_index >= get_size()) {
        return Errors::IndexOutOfBounds;
      }

      if (get_size() != target.get_size()) {
        return Errors::SizeMismatch;
      }

      return get_fitted_at(target, 0, target_index);
    }
    virtual constexpr auto get_fitted_at(
        const SemanticLayout& target,
        Count target_offset,
        Count target_index) const
        -> Perimortem::Utility::Result<const Abstract&, Errors> = 0;

    constexpr auto is_empty() const -> Bool { return get_size() == 0; }

   protected:
    constexpr auto has_target_segment(
        const SemanticLayout& target,
        Count target_offset) const -> Bool {
      return target_offset <= target.get_size() &&
             get_size() <= target.get_size() - target_offset;
    }
  };

  // SemanticPack carries one produced semantic flow without becoming another
  // graph identity. Its SemanticLayout names the real Abstracts supplied by the
  // producer.
  class SemanticPack {
   public:
    constexpr virtual ~SemanticPack() = default;
    virtual constexpr auto get_layout() const -> const SemanticLayout& = 0;
  };

  // QueryContext owns one caller's observation results. Packing copies snapshot
  // shape and names while every Abstract edge continues to borrow its owner.
  class QueryContext {
   public:
    constexpr virtual ~QueryContext() = default;
    virtual auto pack(const SemanticLayout& layout) -> const SemanticPack& = 0;
  };

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

  // Returns the exact Type fact established for this identity. None proves
  // that the identity has no Type. Unknown preserves an answer that may still
  // materialize while the graph is completing.
  virtual auto get_type() const -> const Abstract&;

  // Resolves one binary concept owned by this Abstract. Concrete languages
  // compose their own concepts one question at a time; TTX does not flatten
  // member access, receiver policy, or invocation into routing modes.
  virtual auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Abstract&;

  // Produces one fresh factual snapshot of the concepts this Abstract can
  // currently answer. Concept order has no semantic meaning.
  virtual auto get_concepts(QueryContext& context) const -> const SemanticPack&;

  // Explicit erased values ask the candidate owner whether it satisfies one
  // exact semantic requirement. The answer records only the higher order
  // relationship. Interface negotiation and every physical Projection remain
  // with the concrete language and Terminal that understand them.
  virtual auto satisfies(const Abstract& requirement) const -> Bool;

  // Returns the documentation visible at this exact Abstract. The concrete
  // object may own authored prose, expose a generated comment, forward another
  // object's documentation, or compose several sources. This query does not
  // resolve identity implicitly.
  //
  // The returned object and every borrowed line remain valid for the lifetime
  // of this Abstract. Missing documentation is represented by an empty
  // Documentation object, never Unknown or a nullable reference.
  virtual constexpr auto get_documentation() const
      -> const Concept::Documentation& = 0;
};

using Layout = Abstract::SemanticLayout;
using Pack = Abstract::SemanticPack;
using Context = Abstract::QueryContext;

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
