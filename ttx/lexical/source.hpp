// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Ttx::Lexical {

// Source pairs a diagnostic path with the text that produced it.
//
// Parsers and later source consumers often need both values together. Keeping
// the pair as a lexical value makes those APIs pass the real source facts
// around instead of inventing tool-specific context wrappers.
class Source {
 public:
  Source() = default;
  Source(
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes text)
      : path(path), text(text) {}

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }
  constexpr auto get_text() const -> Perimortem::Core::View::Bytes {
    return text;
  }

 private:
  Perimortem::Core::View::Bytes path;
  Perimortem::Core::View::Bytes text;
};

}  // namespace Ttx::Lexical
