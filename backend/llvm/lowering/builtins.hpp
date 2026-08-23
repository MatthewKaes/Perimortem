// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "backend/llvm/lowering/execution.hpp"
#include "llvm-c/Types.h"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Backend::Llvm::Lowering {

// Builtins maps generated Library Callables to target operations. Their
// signatures and folding remain semantic Library facts while native calling
// details stay on this side of the terminal boundary.
class Builtins {
 public:
  static auto lower(
      const Execution& execution,
      const Ttx::Model::Callable& callable,
      const Ttx::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<const Ttx::Model::Pack&> receiver_source)
      -> Perimortem::Core::Option<Bool>;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Lowering
