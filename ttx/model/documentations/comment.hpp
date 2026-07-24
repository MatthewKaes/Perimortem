// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/documentation.hpp"

namespace Ttx::Model::Documentations {

// Comment exposes one stable line of documentation. It is useful for concepts
// whose explanation is known by the implementation rather than collected from
// source. The binary-wide empty Comment is the canonical documentation dead
// end, matching Invalid's canonical access pattern without introducing another
// namespace-level sentinel.
class Comment : public Concept::Documentation {
 public:
  static auto get_empty() -> const Comment&;

  explicit constexpr Comment(Perimortem::Core::View::Bytes text) : text(text) {}

  constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes override {
    return index == 0 ? text : Perimortem::Core::View::Bytes();
  }

  constexpr auto line_count() const -> Count override {
    return text.is_empty() ? 0 : 1;
  }

 private:
  Perimortem::Core::View::Bytes text;
};

}  // namespace Ttx::Model::Documentations
