// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System::Compression {

class Deflate {
 public:
  enum class Level : Bits_8 {
    None = 0,     // Fastest
    Default = 1,  // Solid compression at a moderate cost
    Best = 2,     // Large compute cost, but moderate improvements
  };

  // Decompresses a deflate stream (RFC 1950 + RFC 1951).
  // Returns empty bytes on malformed input or checksum failure.
  // capacity_hint pre-reserves the output buffer when the decompressed size is
  // known ahead of time, eliminating reallocation cascades on large inputs.
  static auto inflate(Core::View::Bytes source, Count capacity_hint = 0)
      -> Memory::Dynamic::Bytes;

  // Compresses data to a deflate stream (RFC 1950 + RFC 1951).
  // Level::None stores raw blocks; higher levels apply LZ77 + Huffman coding.
  static auto deflate(Core::View::Bytes source, Level level = Level::Default)
      -> Memory::Dynamic::Bytes;
};

}  // namespace Perimortem::System::Compression
