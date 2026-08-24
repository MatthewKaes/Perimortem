// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/graphics/size_2d.hpp"
#include "perimortem/graphics/tone.hpp"

namespace Perimortem::Graphics::Frame {

// Sprite is the backend neutral input copied for one textured quad draw. The
// hosted Object keeps its richer identity while the frame retains the source
// image extent, displayed size, and tone beside its composed transform and
// pixel resource.
struct Sprite {
  Size2D image_size_pixels;
  Size2D size_pixels;
  Tone tone;
};

static_assert(__is_trivially_copyable(Sprite));
static_assert(__is_standard_layout(Sprite));

}  // namespace Perimortem::Graphics::Frame
