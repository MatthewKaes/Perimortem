// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Ttx {

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
  constexpr auto line_at(Count index) const -> Perimortem::Core::View::Bytes {
    return index < lines.get_size() ? lines[index]
                                    : Perimortem::Core::View::Bytes();
  }
  constexpr auto is_empty() const -> Bool { return lines.is_empty(); }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Ttx
