// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/bitflag.hpp"
#include "perimortem/memory/view/bytes.hpp"

#include <bit>
#include <filesystem>
#include <fstream>

namespace Perimortem::Storage::Archive {
using HeaderBlock = Bits_32;
using SizeBlock = Bits_64;

constexpr Byte max_run_length_bytes = sizeof(SizeBlock) + 1;

constexpr HeaderBlock autogenetic_header = std::byteswap('PM-A');

enum class Type : HeaderBlock {
  // Binary format useful for basic light weight storage.
  Standard = std::byteswap('/std'),
  // Binary format that is compressed for reduced size.
  Compressed = std::byteswap('/@_?'),

  // Binary format that only loads the data blocks on request.
  // Useful if you the data volumn is huge and you only want to access part of
  // it.
  Streamed = std::byteswap('/==>'),
  // Binary format that only loads the data blocks on request, but each block is
  // compressed. Useful if you the data volumn is huge and you only want to
  // access part of it.
  StreamedCompressed = std::byteswap('/@_>'),

  // A single inline block optimized for loading multiple virtual files as
  // quickly as possible. All virtualized files are compressed as a single
  // block.
  Memory = std::byteswap('/==?'),
};

enum class CompressionLevels : Int {
  Fastest = -7,  // Recommended for editing.
  Fast = 1,
  Default = 3,
  Small = 9,
  Smallest = 22,  // Recomended for export of the file system to production.
};

constexpr View::Bytes virutal_extension = "autogenetic"_view;
constexpr std::endian file_endian = std::endian::little;

// Blob Options
enum class StorageOptions : Byte {
  Preload,
  Evict,  // Express the intent to unload the resource
  ReadOnly,

  // Enable bit flags.
  TOTAL_FLAGS,
  None = static_cast<Byte>(-1),
};

using StorageFlags = Memory::View::BitFlag<StorageOptions>;

struct FileData {
  Memory::View::Bytes path;
  Memory::View::Bytes data;
  StorageFlags options = StorageOptions::None;
};

}  // namespace Perimortem::Storage::Archive