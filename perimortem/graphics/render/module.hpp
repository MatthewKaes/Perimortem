// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/graphics/render/stage.hpp"

namespace Perimortem::Graphics::Render {

// A borrowed shader module selected for one graphics pipeline stage. Keeping
// the compiled module as 32-bit words preserves its alignment and lets a
// backend derive the byte count without trusting a second size value. An empty
// entry name selects the conventional `main` entry point.
struct Module {
  Stage stage = Stage::Vertex;
  Core::View::Vector<U32> words;
  Core::View::Bytes entry;
};

}  // namespace Perimortem::Graphics::Render
