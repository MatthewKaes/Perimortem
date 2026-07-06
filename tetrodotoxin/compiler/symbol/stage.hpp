// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Symbol {

// Stage describes one emitted shader stage blob in the read-only data buffer.
// The name becomes the linker-visible data symbol for the SPIR-V module.
class Stage {
 public:
  Perimortem::Core::View::Bytes name;
  Perimortem::Utility::Range range;
};

}  // namespace Tetrodotoxin::Compiler::Symbol
