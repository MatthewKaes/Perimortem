// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// Projection maps one concrete Shader Instance Object to target ABI facts. The
// record has process lifetime and contains no semantic graph pointer. Program
// selects the compiled Shader product while the byte range selects the exact
// Parameters subobject owned by that Instance.
struct Projection {
  const U8* program;
  Count parameters_offset;
  Count parameters_size;
};

static_assert(__is_trivial(Projection));

}  // namespace Perimortem::Graphics
