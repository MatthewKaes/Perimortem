// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/virtual_disk_writer.hpp"

#include <bit>
#include <chrono>
#include <cstring>
#include <fstream>

#include <zstd.h>

using namespace Perimortem::Storage;

template <typename T>
auto write_block(Bytes& source, T block) -> void {
  using stroage_type = T;
  source.resize(source.size() + sizeof(stroage_type));
  stroage_type data = static_cast<stroage_type>(block);
  if (std::endian::native != file_endian) {
    data = std::byteswap(data);
  }

  *(stroage_type*)(&source[source.size() - sizeof(stroage_type)]) = data;
}

template <typename T>
auto overwrite_block(Byte* source, T block) -> void {
  using stroage_type = T;
  stroage_type data = static_cast<stroage_type>(block);
  if (std::endian::native != file_endian) {
    data = std::byteswap(static_cast<stroage_type>(data));
  }

  Byte* byte_form = (Byte*)&data;
  for (int i = 0; i < sizeof(stroage_type); i++) {
    source[i] = byte_form[i];
  }
}

auto write_size(Bytes& source, SizeBlock size) -> void {
  // Run length size encoding
  if (size < std::numeric_limits<uint8_t>::max()) {
    write_block<char>(source, sizeof(uint8_t));
    write_block<uint8_t>(source, size);
  } else if (size < std::numeric_limits<uint16_t>::max()) {
    write_block<char>(source, sizeof(uint16_t));
    write_block<uint16_t>(source, size);
  } else if (size < std::numeric_limits<uint32_t>::max()) {
    write_block<char>(source, sizeof(uint32_t));
    write_block<uint32_t>(source, size);
  } else {
    write_block<char>(source, sizeof(uint64_t));
    write_block<uint64_t>(source, size);
  }
}

auto write_stream(Bytes& source, const Byte* start, SizeBlock size) -> void {
  // Run length size encoding
  write_size(source, size);
  source.reserve(source.size() + size);
  source.insert(source.end(), start, start + size);
}

auto VirtualDiskWriter::create(DiskType format_, CompressionLevels compression_)
    -> void {
  format = format_;
  compression = compression_;
  switch (format) {
    // Split header and data table so create additional data block.
    case DiskType::Standard:
    case DiskType::Compressed:
    case DiskType::Streamed:
    case DiskType::StreamedCompressed:
      data_blocks.resize(2);
      break;

    // Inline headers which require reading the entire block.
    case DiskType::Memory:
      data_blocks.resize(1);
      break;

    // Something is wrong and we have a corrupted disk state.
    // Default to a standard disk to attempt a graceful recover.
    default:
      // TODO: Error handling.
      __builtin_debugtrap();

      format = DiskType::Standard;
      data_blocks.resize(2);
      break;
  }

  // Create the format block
  overwrite_block(format_block, autogenetic_header);
  overwrite_block(format_block + sizeof(HeaderBlock),
                  static_cast<HeaderBlock>(format));
};

auto VirtualDiskWriter::write_resource(const std::string_view& p,
                                       const Byte* data,
                                       const SizeBlock size,
                                       StorageFlags flags) -> void {
  const auto& entry = p;

  switch (format) {
    // Split header and data table so create additional data block.
    case DiskType::Standard:
    case DiskType::Compressed:
    case DiskType::Streamed:
    case DiskType::StreamedCompressed:
      data_blocks.resize(2);
      break;

    // Inline headers which require reading the entire block.
    case DiskType::Memory:
      data_blocks.resize(1);
      break;
  }
  write_stream(data_blocks[0], (Byte*)entry.data(), entry.size());
  write_block(data_blocks[0], flags.raw());

  // If the file has been virtualized then we need to store it in the disk along
  // with the resource path.

  if (flags.has(StorageOptions::Virtualized)) {
    int target_block = 0;  // inline
    switch (format) {
      case DiskType::Standard:
      case DiskType::Compressed:
      case DiskType::Streamed:
      case DiskType::StreamedCompressed:
        // The actual file location needs to be back filled later.
        write_size(data_blocks[target_block], data_blocks[1].size());

        // Split header and data table so create additional data block.
        target_block = 1;  // output to target.
        break;

      // Inline headers just need to write the size
      case DiskType::Memory:
        // Don't do anything special for inline blocks.
        break;
    }

    // Write out the data blob.
    // For streaming compress data we need to compress the blob.
    if (format == DiskType::StreamedCompressed) {
      auto view = std::span<const Byte>(data, size);
      Bytes comp_block;
      compress(comp_block, view);
      write_stream(data_blocks[target_block], comp_block.data(),
                   comp_block.size());
    } else {
      write_stream(data_blocks[target_block], data, size);
    }
  }

  // Count the file as stored.
  files_stored += 1;
};

auto VirtualDiskWriter::write_disk(const std::filesystem::path& p) -> bool {
  // Fully compress the block.
  // Note that StreamedCompressed is compressed at the file level, not the block
  // level.
  if (format == DiskType::Compressed || format == DiskType::Memory) {
    for (auto& block : data_blocks) {
      // Attempt to compress block.
      auto success = compress(block);

      // If compression fails then update the header byte.
      if (!success) {
        // TODO: Error handling.
        __builtin_debugtrap();
      }
    }
  }

  std::filesystem::path final_path = p;
  final_path.replace_extension(virutal_disk_extension);
  std::ofstream storage_stream(final_path, std::ios::binary);
  if (!storage_stream.is_open()) {
    // TODO: Error handling.
    __builtin_debugtrap();
  }

  // Write the format information to disk.
  storage_stream.write((char*)format_block, sizeof(format_block));

  // Write all of the blocks to disk.
  for (auto& block : data_blocks) {
    // Don't run length encode the size of a block as it allows us to snag it in
    // one read rather than two (one for run length, one for actual bytes).
    SizeBlock block_size = block.size();
    storage_stream.write((char*)&block_size, sizeof(SizeBlock));

    if (std::endian::native != file_endian) {
      block_size = std::byteswap(block_size);
    }

    storage_stream.write((char*)block.data(), block.size());
  }

  return true;
}

auto VirtualDiskWriter::compress(Bytes& source) -> bool {
  Bytes compressed;
  if (!compress(compressed, ByteView(source.data(), source.size())))
    return false;

  source.swap(compressed);

  return true;
}

auto VirtualDiskWriter::compress(Bytes& output, ByteView source) -> bool {
  const int level =
      static_cast<std::underlying_type_t<CompressionLevels>>(compression);
  static auto cctx = ZSTD_createCCtx();
  if (cctx == nullptr) {
    // TODO: Error handling.
    __builtin_debugtrap();
    return false;
  }

  // TODO: Streaming compression to save on memory.
  output.resize(ZSTD_compressBound(source.size()));

  size_t const comp_size = ZSTD_compressCCtx(
      cctx, output.data(), output.size(), source.data(), source.size(), level);
  output.resize(comp_size);

  // Compression failed for some reason.
  // if (comp_size > source.size())
  //   return false;

  return true;
}