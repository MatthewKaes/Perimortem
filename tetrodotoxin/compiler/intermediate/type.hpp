// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Compiler::Intermediate {

// Represents a type lowering from TTX to generic ISA
// TODO: This is a major hack and should all be moved out to an actual IR.
class Type {
 public:
  // The mechanism which to lower the type with.
  enum class Lowering {
    // Unreachable
    Void,
    // 8 BYTE*, 8 BITS_64
    ViewBytes,
  };

  static auto type_void() -> Type {
    return {
      "void"_view,
      Lowering::Void,
    };
  }

  static auto type_view() -> Type {
    return {
      "Perimortem::Core::View::Bytes"_view,
      Lowering::ViewBytes,
    };
  }

  Perimortem::Core::View::Bytes name;
  Lowering lowering;
};

}  // namespace Tetrodotoxin::Compiler::Intermediate
