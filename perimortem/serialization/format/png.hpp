// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/graphics/image.hpp"

namespace Perimortem::Serialization::Format::Png {

// Decodes a Png byte stream into an Image (width × height RGBA pixels,
// row-major). All source color formats are expanded to full RGBA.
// Returns an empty Image on malformed or unsupported input.
auto decode(Core::View::Bytes source) -> Graphics::Image;

// Encodes an Image to a Png byte stream. Returns empty bytes on failure.
auto encode(const Graphics::Image& image) -> Memory::Dynamic::Bytes;

}  // namespace Perimortem::Serialization::Format::Png
