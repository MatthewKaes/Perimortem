// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Ttx {

// Documentation is source-authored prose collected from consecutive lexical
// comment tokens. A documentation object is the language-owned home for that
// prose.
//
// TTX packages need to preserve the explanations authors wrote beside the thing
// being declared. An editor can surface documentation in hover text. A
// formatter can keep it attached to the next declaration. A package exporter
// can carry it forward without inventing a second documentation model outside
// the language.
//
// Documentation is intentionally not canonicalized because prose describes the
// name and context being used, not only the root object beneath it. A canonical
// Sprite type can document the actual graphics object. An alias such as
// HudSprite can document the UI specific promises that matter when the same
// Sprite type is used in a heads up display. A member can document why `.tint`
// exists on that containing type, and a function can document the behavior
// promised by one callable entry.
//
// That layering gives tools useful choices. They can show alias documentation
// directly, ignore it and show the canonical type documentation, or stack both
// so the alias amends the canonical prose with details from the aliased
// context. The model keeps those choices open by storing documentation on the
// thing that authored it.
//
// Rendering, markup interpretation, and canonical documentation merging are
// consumer choices. Editors, exporters, and package tools can choose the
// presentation that fits their surface because this class only preserves
// authored lines and keeps them attached to the TTX concept that owns them.
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
