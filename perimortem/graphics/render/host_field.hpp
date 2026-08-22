// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Graphics::Render {

// Identifies one named field in the host-input layout. The offset and size are
// retained as reflection data so callers can inspect or populate a layout
// without exposing compiler types to the graphics runtime.
struct HostField {
  Core::View::Bytes name;
  Count offset = 0;
  Count size = 0;
};

}  // namespace Perimortem::Graphics::Render
