// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"
#include "ttx/ffi/cpp/callable.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Builtins maps generated Library Callables to target operations. Their
// signatures and folding remain with their Library owners, while native
// calling details stay on this side of the Terminal boundary.
class Builtins {
 public:
  static auto lower(
      const Execution& execution,
      const Ttx::Model::Callable& callable,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Pack&> receiver_source)
      -> Perimortem::Core::Option<Bool>;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
