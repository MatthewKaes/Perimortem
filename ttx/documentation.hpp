// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Ttx {

// Documentation is a core part of the TTX spec and is part of the type system.
//
// In a typical language multiple tools need to decide how to handle comments
// and documentation across tools. Having documentation be a first class citizen
// in TTX instead of just a source code concept unlocks a lot of nice to have
// features for tooling:
//
// * TTX packages can require a document comment natively.
// * Dialects can easily attach a common documentation format if desired.
// * Type resoltuion can propagate documentation, such as through aliases.
// * Formatting doesn't need to special case documentation.
//
// Documentation is intentionally not canonicalized since the context it's
// defined in is as much part of the documentation as the text itself. Graphic's
// canonical Sprite type can document the actual graphics object. An alias such
// as HudSprite can document the UI specific promises that matter when the same
// Sprite type is used. A member can document the color format for `.tint` on a
// containing type, and a function can document the behavior promised by one
// callable entry.
//
// While this layering is required to be representable in the TTX data model
// tools can perform whatever projections they wish with it. They can show alias
// documentation directly, ignore it and show the canonical type documentation,
// or stack both so the alias amends the canonical prose with details from the
// aliased context.
class Documentation {
 public:
  constexpr Documentation() = default;
  explicit constexpr Documentation(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines)
      : lines(lines) {}

  constexpr auto get_lines() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return lines;
  }
  constexpr auto get_line_count() const -> Count { return lines.get_size(); }

  // Returns empty bytes when the line does not exist. Documentation lookup uses
  // absence as the bottom case because missing prose is not a parse error.
  constexpr auto line_at(Count index) const -> Perimortem::Core::View::Bytes {
    return index < lines.get_size() ? lines[index]
                                    : Perimortem::Core::View::Bytes();
  }
  constexpr auto is_empty() const -> Bool { return lines.is_empty(); }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Ttx
