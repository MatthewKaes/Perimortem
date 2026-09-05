// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Values lowers completed Library Constants into the carrier selected for this
// native module. Library continues to own what each Constant means; this
// Terminal owns only its LLVM representation.
class Values {
 public:
  static auto lower(
      const Execution& execution,
      const Ttx::Concept::Abstract& value) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
