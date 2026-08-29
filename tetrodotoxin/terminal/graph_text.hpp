// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/product.hpp"
#include "ttx/concept/abstract.h"

namespace Tetrodotoxin::Terminal {

// GraphText emits one dialect-neutral, lossless description of the Abstract
// graph visible to a caller. The dump is a presentation product: it borrows no
// semantic ownership and assigns IDs only for this serialization transaction.
class GraphText {
 public:
  static auto write(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source,
      const ttx_abstract* dialect,
      const ttx_abstract* root,
      const ttx_abstract* graph) -> const Tetrodotoxin::Language::Product&;
};

}  // namespace Tetrodotoxin::Terminal
