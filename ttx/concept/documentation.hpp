// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Ttx::Concept {

// Documentation preserves source-authored prose in source order. It is a
// borrowed first-class concept, not an Abstract or semantic identity. Any TTX
// object whose authored context needs an explanation can own the value without
// changing resolution, Type equivalence, Layout equivalence, or fitting.
//
// Context is intentional. An Alias can explain why its local name redirects
// to a target while that target retains independent documentation. Package
// groups, members, callables, and ISA-owned objects can make the same choice.
// Consumers decide whether to present local prose, resolved prose, or both.
//
// The owner of a Documentation value must keep the line storage and referenced
// source bytes alive. Missing prose is an empty view, not Invalid, because
// documentation is optional presentation rather than a failed semantic query.
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

  constexpr auto line_at(Count index) const -> Perimortem::Core::View::Bytes {
    return index < lines.get_size() ? lines[index]
                                    : Perimortem::Core::View::Bytes();
  }

  constexpr auto is_empty() const -> Bool { return lines.is_empty(); }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Ttx::Concept
