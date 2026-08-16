// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Ttx::Concept {

// Documentation preserves text form in presentation order. It is a borrowed
// first-class concept, not an Abstract or semantic identity. Abstracts expose
// this contract directly as treating documentation as a universal query saves
// a ton of headache down the road compared to models that treat documentation
// as an optional chunk of metadata.
//
// Documentation allows for wrapping and interception. Alias for instance
// either forward their target's documentation directly or use a `Merged` node
// to wrap their local documentation on the target's documentation. This creates
// a direct way for layering and enriching documentation context just like any
// other system inside of TTX.
//
// Implementations may own source authored lines, generate a stable comment, or
// combine documentation from several Abstracts. They must keep every borrowed
// line alive for as long as this object is queryable. Missing documentation is
// empty and does not resolve to to an Invalid since presentation is optional
// rather than a failed semantic query.
//
// Formaters and Serializers should be cautious when querying documentation and
// should not assume that all documentation follows a naive schema and should
// instead treat it as a proper contractual heirarchy to avoid needless amounts
// of duplication.
class Documentation {
 public:
  constexpr virtual ~Documentation() = default;

  static auto get_empty() -> const Documentation&;

  virtual constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes {
    return {};
  }

  virtual constexpr auto line_count() const -> Count { return 0; }

  virtual constexpr auto is_empty() const -> Bool { return line_count() == 0; }
};

}  // namespace Ttx::Concept

#define TTX_EMPTY_DOCUMENTATION()                      \
  auto get_documentation() const                       \
      -> const Ttx::Concept::Documentation& override { \
    return Ttx::Concept::Documentation::get_empty();   \
  }

#define TTX_DOCUMENTATION(expression)                  \
  constexpr auto get_documentation() const             \
      -> const Ttx::Concept::Documentation& override { \
    return expression;                                 \
  }
