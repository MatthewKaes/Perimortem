// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System::Compression {

enum class Level : Bits_8 {
  None = 0,     // Fastest
  Default = 1,  // Solid compression at a moderate cost
  Best = 2,     // Large compute cost, but moderate improvements
};

// Decompresses a deflate stream (RFC 1950 + RFC 1951).
// Returns empty bytes on malformed input or checksum failure.
auto inflate(Core::View::Bytes source) -> Memory::Dynamic::Bytes;

// Compresses data to a deflate stream (RFC 1950 + RFC 1951).
// Level::None stores raw blocks; higher levels apply LZ77 + Huffman coding.
auto deflate(Core::View::Bytes source, Level level = Level::Default)
    -> Memory::Dynamic::Bytes;

}  // namespace Perimortem::System::Compression
