// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Abi::Graphics {

// Size2D is the trivial native carrier for the corresponding Graphics value.
// Keeping the two dimensions explicit lets provider code agree with generated C
// declarations without borrowing the C++ value's construction behavior.
class Size2D {
 public:
  static constexpr auto create(U32 width, U32 height) -> Size2D {
    Size2D result = {};
    result.width = width;
    result.height = height;
    return result;
  }

 private:
  U32 width;
  U32 height;
};

static_assert(sizeof(Size2D) == sizeof(U32) * 2);
static_assert(alignof(Size2D) == alignof(U32));
static_assert(__is_trivial(Size2D));
static_assert(__is_standard_layout(Size2D));

}  // namespace Perimortem::Abi::Graphics
