// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "backend/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Backend::Llvm::Lowering {

// Values translates completed immutable Library values into the carrier facts
// selected by this backend. Constant domains remain entirely owned by Library.
class Values {
 public:
  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Expression& expression) -> Bool;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Lowering
