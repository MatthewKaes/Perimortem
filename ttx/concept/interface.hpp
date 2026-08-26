// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Pack and Layout form TTX's vocabulary for data flow. A Pack retains the real
// producers of its values, while a Layout projects those values into the shape
// a consumer may receive. Fitting then maps the source flow into the target
// flow. This projection deliberately forgets behavior and domain policy, so
// two Abstracts can have equivalent Layouts without being semantically
// interchangeable.
//
// Interface exists for the relation that projection cannot express. It asks
// whether one real Abstract can occupy the semantic role required by another
// without translating either identity or teaching their Dialects about each
// other. In category theory terms, Pack and Layout provide the structure for
// data flow morphisms, while Interface supplies a higher order relation between
// the objects at their boundaries.
//
// A concrete Interface may use Layout fitting as evidence, then add the meaning
// that fitting deliberately omits. Callable compatibility can consider inputs,
// results, and behavioral policy. Other domains can negotiate resources,
// lifecycles, or relationships that carry no value flow at all. Questions that
// concern only value shape remain with Layout, which keeps Interface focused on
// semantic substitution. The negotiator retains no copied semantic inventory
// and creates no graph identity, so the two Abstracts remain the only sources
// of meaning.
//
// Negotiation also creates no runtime representation. A concrete language may
// retain an accepted candidate in an explicit erased value, after which its
// Terminal derives the Projection required by the selected ABI. Known concrete
// candidates keep their direct representation and pay no dispatch cost merely
// because an Interface accepted them.
class Interface {
 public:
  enum class Relation : U8 {
    Rejected,
    Satisfied,
    Equivalent,
  };

  constexpr virtual ~Interface() = default;

  // Direction matters: candidate attempts to satisfy requirement. Equivalent
  // is reserved for a relationship the concrete negotiator proves in both
  // directions.
  virtual constexpr auto negotiate(
      const Abstract& requirement,
      const Abstract& candidate) const -> Relation = 0;

  constexpr auto accepts(const Abstract& requirement, const Abstract& candidate)
      const -> Bool {
    return negotiate(requirement, candidate) != Relation::Rejected;
  }

  constexpr auto equivalent(const Abstract& left, const Abstract& right) const
      -> Bool {
    return negotiate(left, right) == Relation::Equivalent;
  }
};

}  // namespace Ttx::Concept
