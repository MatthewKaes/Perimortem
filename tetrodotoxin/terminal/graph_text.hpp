// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstdint>
#include <vector>

#include "ttx/abi.h"

namespace Tetrodotoxin::Terminal {

// Graph Text is the first Terminal which consumes the canonical graph without
// selecting a native C++ owner. It copies the answers exposed during one
// observation into a deterministic textual report, including partial Domain
// answers and the complete Layout topology available to the caller. The bytes
// are a product of the observation rather than another semantic participant;
// they never become an authority that the live graph resolves through later.
class GraphText {
 public:
  using Bytes = std::vector<uint8_t>;

  static auto write(
      ttx_borrowed_bytes source,
      ttx_abstract dialect,
      ttx_abstract root,
      ttx_abstract graph) -> Bytes;
};

}  // namespace Tetrodotoxin::Terminal
