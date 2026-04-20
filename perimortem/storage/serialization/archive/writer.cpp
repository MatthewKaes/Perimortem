// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/storage/serialization/archive/writer.hpp"

#include <zstd.h>

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Archive;

auto write_size(Dynamic::Bytes& source, SizeBlock size) -> void {
  // Run length size encoding
  if (size <= Bits_8(-1)) {
    source.serialize(Bits_8(sizeof(Bits_8)));
    source.serialize(Bits_8(size));
  } else if (size <= Bits_16(-1)) {
    source.write<Bits_16>(sizeof(Bits_16));
    source.write<Bits_16>(size);
  } else if (size <= Bits_32(-1)) {
    source.write<Bits_32>(sizeof(Bits_32));
    source.write<Bits_32>(size);
  } else {
    source.write<Bits_64>(sizeof(Bits_64));
    source.write<Bits_64>(size);
  }
}

auto write_stream(Managed::Bytes& source, const View::Bytes data) -> void {
  // Run length size encoding
  write_size(source, data.get_size());
  source.write(data);
}

auto Writer::create(Type format_, CompressionLevel compression_) -> void {
  format = format_;
  compression = compression_;
  switch (format) {
    // Split header and data table so create additional data block.
    case Type::Standard:
    case Type::Compressed:
    case Type::Streamed:
    case Type::StreamedCompressed:
      data_blocks.insert(Perimortem::Memory::Managed::Bytes(disk_arena));
      data_blocks.insert(Perimortem::Memory::Managed::Bytes(disk_arena));
      break;

    // Inline headers which require reading the entire block.
    case Type::Memory:
      data_blocks.insert(Perimortem::Memory::Managed::Bytes(disk_arena));
      break;

    // Something is wrong and we have a corrupted disk state.
    // Default to a standard disk to attempt a graceful recover.
    default:
      // TODO: Error handling.
      __builtin_debugtrap();

      format = Type::Standard;
      data_blocks.insert(Perimortem::Memory::Managed::Bytes(disk_arena));
      data_blocks.insert(Perimortem::Memory::Managed::Bytes(disk_arena));
      break;
  }
};

auto Writer::stage_resource(Memory::View::Bytes p,
                            Memory::View::Bytes data,
                            FileFlags flags) -> void {
  const auto& entry = p;

  write_stream(data_blocks[0], entry);
  data_blocks[0].write(flags);

  // If the file has been virtualized then we need to store it in the disk along
  // with the resource path.

  if (flags.has(FileStorage::Virtualized)) {
    int target_block = 0;  // inline
    switch (format) {
      case Type::Standard:
      case Type::Compressed:
      case Type::Streamed:
      case Type::StreamedCompressed:
        // The actual file location needs to be back filled later.
        write_size(data_blocks[target_block], data_blocks[1].get_size());

        // Split header and data table so create additional data block.
        target_block = 1;  // output to target.
        break;

      // Inline headers just need to write the size
      case Type::Memory:
        // Don't do anything special for inline blocks.
        break;
    }

    // Write out the data blob.
    // For streaming compress data we need to compress the blob.
    if (format == Type::StreamedCompressed) {
      compress(data_blocks[target_block], data);
    } else {
      write_stream(data_blocks[target_block], data);
    }
  }
};

auto Writer::write_disk() -> View::Bytes {
  Count max_size = 0;
  for (Count i = 0; i < data_blocks.get_size(); i++) {
    max_size += data_blocks[i].get_size();
  }
  Managed::Bytes disk_output(disk_arena);
  disk_output.ensure_capacity(max_size);

  // Write the format information to disk.
  disk_output.write(autogenetic_header);
  disk_output.write(format);

  // Write all of the blocks to disk.
  Allocator::Arena compress_arena;
  for (Count i = 0; i < data_blocks.get_size(); i++) {
    compress_arena.reset();
    auto active_block = data_blocks[i];
    // Fully compress the block.
    // Note that StreamedCompressed is compressed at the file level, not the
    // block level.
    if (format == Type::Compressed || format == Type::Memory) {
      // Attempt to compress block.
      // Use a fresh Arena so the blob doesn't stick around.
      Managed::Bytes compressed_block(compress_arena);
      auto success = compress(compressed_block, active_block);
      active_block = compressed_block;

      // If compression fails then update the header byte.
      if (!success) {
        // TODO: Error handling.
        __builtin_debugtrap();
      }
    }

    // Don't run length encode the size of a block as it allows us to snag it
    // in one read rather than two (one for run length, one for actual bytes).
    SizeBlock block_size = data_blocks[i].get_size();
    if (std::endian::native != file_endian) {
      block_size = std::byteswap(block_size);
    }
    disk_output.write(block_size);
    disk_output.append(active_block.get_view());
  }

  return disk_output;
}

auto compress(Dynamic::Bytes& output, View::Bytes source, CompressionLevel compression) -> Bool {
  const Int level =
      static_cast<std::underlying_type_t<CompressionLevel>>(compression);
  thread_local static auto cctx = ZSTD_createCCtx();
  if (cctx == nullptr) {
    // TODO: Error handling.
    __builtin_debugtrap();
    return false;
  }

  // Estimate size storage
  Count upper_bound = ZSTD_compressBound(source.get_size());
  Count size_location = output.get_size();
  if (upper_bound <= Bits_8(-1)) {
    output.write<Bits_8>(sizeof(Bits_8));
    output.write<Bits_8>(upper_bound);
  } else if (upper_bound <= Bits_16(-1)) {
    output.write<Bits_16>(sizeof(Bits_16));
    output.write<Bits_16>(upper_bound);
  } else if (upper_bound <= Bits_32(-1)) {
    output.write<Bits_32>(sizeof(Bits_32));
    output.write<Bits_32>(upper_bound);
  } else {
    output.write<Bits_64>(sizeof(Bits_64));
    output.write<Bits_64>(upper_bound);
  }

  Count offset = output.get_size();
  output.ensure_capacity(upper_bound);

  Count comp_size = ZSTD_compressCCtx(
      cctx, const_cast<Byte*>(output.get_view().get_data()) + offset,
      upper_bound, source.get_data(), source.get_size(), level);
  output.resize(offset + comp_size);

  // Write actual size
  if (upper_bound <= Bits_8(-1)) {
    output.write_at<Bits_8>(comp_size, size_location);
  } else if (upper_bound <= Bits_16(-1)) {
    output.write_at<Bits_16>(comp_size, size_location);
  } else if (upper_bound <= Bits_32(-1)) {
    output.write_at<Bits_32>(comp_size, size_location);
  } else {
    output.write_at<Bits_64>(comp_size, size_location);
  }
  return true;
}