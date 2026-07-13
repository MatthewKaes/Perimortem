// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler {

// Names one linker-visible range in a compiler-produced byte buffer. The
// producer owns the bytes and their meaning. Engine assigns the object section
// before publishing the corresponding linker symbol.
class Symbol {
 public:
  constexpr Symbol(
      Perimortem::Core::View::Bytes name,
      Perimortem::Utility::Range range)
      : name(name), range(range) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_range() const -> Perimortem::Utility::Range {
    return range;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Utility::Range range;
};

}  // namespace Tetrodotoxin::Compiler
