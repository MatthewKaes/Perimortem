// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/llvm/failure.hpp"
#include "tetrodotoxin/library/llvm/products.hpp"
#include "tetrodotoxin/library/llvm/request.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Compiler owns the LLVM Library compilation entry. Every call is one complete
// transaction and publishes products only after verification succeeds.
class Compiler {
 public:
  constexpr Compiler() = default;

  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Request& request) const
      -> Perimortem::Utility::Result<Products, Failure>;
};

}  // namespace Tetrodotoxin::Library::Llvm
