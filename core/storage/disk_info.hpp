// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "concepts/bitflag.hpp"
#include "memory/view/bytes.hpp"
#include "core/concepts/standard_types.hpp"

#include <bit>
#include <filesystem>
#include <fstream>

namespace Perimortem::Storage {
using HeaderBlock = Bits_32;
using SizeBlock = Bits_64;

constexpr uint8_t max_run_length_bytes = sizeof(SizeBlock) + 1;

constexpr HeaderBlock autogenetic_header = std::byteswap('PM-A');

enum class DiskType : HeaderBlock {
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

enum class CompressionLevels : int8_t {
  Fastest =
      -7, // Recommended if you are using for virtualization while editing.
  Fast = 1,
  Default = 3,
  Small = 9,
  Smallest = 22, // Recomended for export of the file system to production.
};

constexpr const char *virutal_disk_extension = "autogenetic";
constexpr std::endian file_endian = std::endian::little;

// Blob Options
enum class StorageOptions : int8_t {
  None = -1,
  Virtualized,
  Preload,
  Streamed,
  ReadOnly,
  TOTAL_FLAGS,
};

using StorageFlags = Concepts::BitFlag<StorageOptions>;

struct FileData {
  Memory::View::Bytes path;
  Memory::View::Bytes data;
  StorageFlags options = StorageOptions::None;
};

} // namespace Perimortem::Storage