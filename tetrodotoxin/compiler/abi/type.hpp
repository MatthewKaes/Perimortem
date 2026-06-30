// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Compiler::Abi {

// Host ABI type spelling for generated function signatures.
//
// This is deliberately not a compiler IR. Real expression lowering should ask
// validation and dialect facts for proven layouts, then cross this boundary
// only when a backend needs to spell a host-callable ABI type.
class Type {
 public:
  enum class Carrier {
    Void,
    ViewBytes,
  };

  static auto void_type() -> Type {
    return {
      "void"_view,
      Carrier::Void,
    };
  }

  static auto view_bytes() -> Type {
    return {
      "Perimortem::Core::View::Bytes"_view,
      Carrier::ViewBytes,
    };
  }

  Perimortem::Core::View::Bytes cpp_name;
  Carrier carrier;
};

}  // namespace Tetrodotoxin::Compiler::Abi
