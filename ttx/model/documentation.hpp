// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Ttx::Model {

// Documentation is a core TTX source and tooling fact. It is not part of
// Abstract identity, Type identity, or Layout fitting.
//
// Keeping documentation as a first-class value lets source-owning declarations
// and contexts expose prose without pushing presentation into semantic
// contracts:
//
// * TTX packages can require a document comment natively.
// * Dialects can easily attach a common documentation format if desired.
// * Formatting doesn't need to special case documentation.
//
// Documentation is intentionally separate from identity resolution because the
// source context is part of the prose. A declaration that introduces HudSprite
// may document why it redirects to Sprite, while Sprite keeps its own
// documentation. Closed Alias owns neither value. The source owner of an
// Addressable can document the color format for `.tint`, and a callable
// declaration can document its local promise.
//
// Tools may show source-context prose, resolved-target prose, or a stacked
// presentation while walking a query. That presentation remains outside Alias
// and does not alter semantic identity.
class Documentation {
 public:
  explicit constexpr Documentation(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines = {})
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

}  // namespace Ttx::Model
