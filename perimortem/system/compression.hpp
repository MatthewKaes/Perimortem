// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System::Compression {

// Decompresses zlib-wrapped DEFLATE data (RFC 1950 & RFC 1951).
// Returns empty bytes on malformed input or checksum failure.
auto inflate(Core::View::Bytes source) -> Memory::Dynamic::Bytes;

// Compresses data to a zlib format but using zero compression.
// TODO: Expand deflate compression, but since this is only used for PNGs for
// now It's not critical to support beyond level-0.
auto deflate(Core::View::Bytes source) -> Memory::Dynamic::Bytes;

}  // namespace Perimortem::System::Compression
