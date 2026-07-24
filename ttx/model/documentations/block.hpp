// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/documentation.hpp"

namespace Ttx::Model::Documentations {

// Comments exposes a borrowed ordered sequence of documentation lines.
// Source evaluators use it when adjacent comment tokens should remain separate
// for proper document reconstruction.
class Block : public Concept::Documentation {
 public:
  explicit constexpr Block(
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines)
      : lines(lines) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    return index < lines.get_size() ? lines[index]
                                    : Perimortem::Core::View::Bytes();
  }

  constexpr auto line_count() const -> Count override {
    return lines.get_size();
  }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> lines;
};

}  // namespace Ttx::Model::Documentations
