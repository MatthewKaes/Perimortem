// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/registry.hpp"
#include "tetrodotoxin/puffer/isa/boot/envelope.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Puffer::Isa::Boot {

// Boot is Puffer's fixed startup ISA for complete source files.
//
// It reads the source preamble and leaves the cursor at the selected body ISA
// once documentation, dialect selection, and imports are known.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Isa::Registry& registry) -> Envelope*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Boot"_view;
  }
};

}  // namespace Tetrodotoxin::Puffer::Isa::Boot
