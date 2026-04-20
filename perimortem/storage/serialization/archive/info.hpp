// Perimortem Engine
// Copyright © Matt Kaes

#pragma once


#include "perimortem/core/standard_types.hpp"
#include "perimortem/memory/view/bytes.hpp"
#include "perimortem/memory/view/bitflag.hpp"
#include "perimortem/system/uuid.hpp"

namespace Perimortem::Storage::Serialization::Archive {
using HeaderBlock = Bits_32;
using SizeBlock = Bits_64;

constexpr Byte max_run_length_bytes = sizeof(SizeBlock) + 1;
constexpr HeaderBlock header = 'A-MP';

enum class Type : Byte {
  // Binary format useful for basic light weight storage.
  Standard,
  // Binary format that is compressed for reduced size.
  Compressed,

  // Binary format that only loads the data blocks on request.
  // Useful if you the data volumn is huge and you only want to access part of
  // it.
  Streamed,
  // Binary format that only loads the data blocks on request, but each block is
  // compressed. Useful if you the data volumn is huge and you only want to
  // access part of it.
  StreamedCompressed,

  // A single inline block optimized for loading multiple virtual files as
  // quickly as possible. All virtualized files are compressed as a single
  // block.
  Memory,
};

enum class CompressionLevel : Int {
  Fastest = -7,  // Recommended for editing.
  Fast = 1,
  Default = 3,
  Small = 9,
  Smallest = 22,  // Recomended for export of the file system to production.
};

// Blob Options
enum class FileStorage : Byte {
  Virtualized,
  Preload,
  Evict,  // Express the intent to unload the resource
  ReadOnly,

  // Enable bit flags.
  TOTAL_FLAGS,
  None = static_cast<Byte>(-1),
};

using FileFlags = Memory::View::BitFlag<FileStorage>;

constexpr auto file_endian = __ORDER_LITTLE_ENDIAN__;

struct Object {
  System::Uuid uuid;
  Memory::View::Bytes path;
  Memory::View::Bytes data;
  FileFlags options = FileStorage::None;
};

}  // namespace Perimortem::Storage::Archive