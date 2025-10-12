// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/virtual_disk_reader.hpp"

#include <bit>
#include <bitset>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>

#include <zstd.h>

using namespace Perimortem::Storage;

template <typename T>
auto read_block(const Byte* source, SizeBlock& pos) -> T {
  T block = *(T*)(&source[pos]);
  if (std::endian::native != file_endian) {
    block = std::byteswap(block);
  }

  pos += sizeof(T);
  return block;
}

template <typename T>
auto read_block(const Byte* source) -> T {
  T block = *(T*)(&source);
  if (std::endian::native != file_endian) {
    block = std::byteswap(block);
  }

  return block;
}

auto read_size(const Byte* source, SizeBlock& pos) -> SizeBlock {
  // Read the run length size encoding byte
  Byte bytes = read_block<Byte>(source, pos);

  switch (bytes) {
    case sizeof(uint8_t):
      return read_block<uint8_t>(source, pos);
    case sizeof(uint16_t):
      return read_block<uint16_t>(source, pos);
    case sizeof(uint32_t):
      return read_block<uint32_t>(source, pos);
    case sizeof(uint64_t):
      return read_block<uint64_t>(source, pos);
  }

  // TODO: Error handling.
  __builtin_debugtrap();
  return -1;
}

// TODO: Virtualize to allow for different OS servers, but this should work for
// all desktops.
class filesystem_reader {
 public:
  filesystem_reader(const std::filesystem::path& disk)
      : disk_data(disk, std::ios::binary | std::ios::ate) {
    disk_size = static_cast<uint64_t>(disk_data.tellg());
    disk_data.seekg(0, std::ios::beg);
  }

  template <typename T>
  auto load_object(T& obj) -> bool {
    constexpr uint64_t size = sizeof(T);

    if (size + bytes_read > disk_size)
      return false;

    disk_data.read((char*)&obj, size);
    bytes_read += size;
    return true;
  }

  auto load(char* data, uint64_t size) -> bool {
    if (size + bytes_read > disk_size)
      return false;

    disk_data.read(data, size);
    bytes_read += size;
    return true;
  }

  auto sync(int64_t size) -> void {
    disk_data.seekg(size, std::ios::cur);
    bytes_read += size;
  }

  auto size() const -> uint64_t { return disk_size; }
  auto remaining() const -> uint64_t { return disk_size - bytes_read; }

  std::ifstream disk_data;
  uint64_t disk_size = 0;
  uint64_t bytes_read = 0;
};

auto VirtualDiskReader::mount_disk(const std::filesystem::path& disk)
    -> std::unique_ptr<VirtualDiskReader> {
  if (!std::filesystem::exists(disk))
    return nullptr;

  filesystem_reader disk_reader(disk);
  if (disk_reader.size() == 0)
    return nullptr;

  // Read the two header blocks.
  Byte format_block[sizeof(HeaderBlock) + sizeof(HeaderBlock)];
  if (!disk_reader.load_object(format_block)) {
    // TODO: Error handling
    __builtin_debugtrap();
    return nullptr;
  }
  std::unique_ptr<VirtualDiskReader> reader =
      std::unique_ptr<VirtualDiskReader>(new VirtualDiskReader());
  reader->disk_path = disk;

  uint64_t format_pos = 0;
  HeaderBlock header = read_block<HeaderBlock>(format_block, format_pos);
  if (header != autogenetic_header) {
    // TODO: Error handling
    __builtin_debugtrap();
  }

  reader->format =
      static_cast<DiskType>(read_block<HeaderBlock>(format_block, format_pos));

  // Allocate the required number of data blocks.
  switch (reader->format) {
    // Split header and data table so create additional data block.
    case DiskType::Standard:
    case DiskType::Compressed:
    // Streamed disk saves memory by loading chunks directly from disk.
    // This avoid having a second copy of the data in memory.
    case DiskType::Streamed:
    case DiskType::StreamedCompressed:
      reader->blocks.resize(2);
      break;

    // Memroy speed reads the data from a single continous buffer.
    case DiskType::Memory:
      reader->blocks.resize(1);
      break;
  }

  for (int i = 0; i < reader->blocks.size(); i++) {
    uint64_t block_size_pos = 0;
    Byte block_size_buffer[sizeof(SizeBlock)];
    if (!disk_reader.load_object(block_size_buffer)) {
      // TODO: Error handling
      __builtin_debugtrap();
      return nullptr;
    }
    SizeBlock block_size =
        read_block<SizeBlock>(block_size_buffer, block_size_pos);

    auto& block_data = reader->blocks[i];
    block_data.location = disk_reader.bytes_read;

    // Load non-empty blocks.
    if (block_size > 0) {
      block_data.data.resize(block_size);

      // For streamed blocks don't read anything beyond the table.
      if (i > 0 && (reader->format == DiskType::Streamed ||
                    reader->format == DiskType::StreamedCompressed)) {
        disk_reader.sync(block_size);
      } else {
        if (!disk_reader.load((char*)block_data.data.data(), block_size)) {
          /* TODO: Error handling. */
          __builtin_debugtrap();
          return nullptr;
        }
      }

      // Decompress blocks.
      switch (reader->format) {
        case DiskType::Compressed:
        case DiskType::Memory:
          decompress_block(block_data.data);
          break;

        // No other disks use compression.
        default:
          break;
      }
    }
  }

  switch (reader->format) {
      // Streamed Disks
    case DiskType::Streamed:
    case DiskType::StreamedCompressed:
    case DiskType::Standard:
    case DiskType::Compressed:
      reader->process_split_table();
      break;

    // Inline headers just need to write the size
    case DiskType::Memory:
      reader->process_inline_table();
      break;
  }

  return reader;
}

auto VirtualDiskReader::stream_from_disk(const std::string_view& path,
                                         Bytes& data) -> bool {
  if (!stream_index.contains(path))
    return false;

  if (!std::filesystem::exists(disk_path))
    return false;

  filesystem_reader disk_reader(disk_path);
  disk_reader.sync(stream_index[path]);

  Byte block_size_buffer[max_run_length_bytes];
  uint64_t run_length = 0;
  if (!disk_reader.load_object(block_size_buffer)) {
    // TODO: Error handling
    __builtin_debugtrap();
    return false;
  }
  SizeBlock block_size = read_size(block_size_buffer, run_length);

  // Adjust to the correct position so we are at the start of the buffer.
  disk_reader.sync(run_length - max_run_length_bytes);

  data.resize(block_size);
  if (!disk_reader.load((char*)data.data(), block_size)) {
    // TODO: Error handling
    __builtin_debugtrap();
    return false;
  }

  // Decompress the streamed data.
  if (format == DiskType::StreamedCompressed) {
    decompress_block(data);
  }

  return true;
}

auto VirtualDiskReader::decompress_block(Bytes& data) -> void {
  static auto dctx = ZSTD_createDCtx();
  if (dctx == nullptr) {
    // TODO: Error handling.
    __builtin_debugtrap();
  }

  uint64_t inflate_size = ZSTD_getFrameContentSize(data.data(), data.size());
  Bytes decompressed(inflate_size);

  ZSTD_decompressDCtx(dctx, decompressed.data(), decompressed.size(),
                      data.data(), data.size());

  // Swap data to the new buffer and then hand off to the next layer.
  data.swap(decompressed);
}

auto VirtualDiskReader::process_split_table() -> void {
  Block& table = blocks[0];
  Block& data = blocks[1];
  uint64_t table_pos = 0;

  while (table_pos < table.data.size()) {
    FileData file;

    SizeBlock name_size = read_size(table.data.data(), table_pos);
    file.path = {(char*)table.data.data() + table_pos, name_size};
    table_pos += name_size;
    file.options = read_block<Byte>(table.data.data(), table_pos);

    if (file.options == StorageOptions::Virtualized) {
      SizeBlock location = read_size(table.data.data(), table_pos);

      SizeBlock data_size = -1;
      // Stream disks don't preload any data, even when virtualized.
      if (format == DiskType::Streamed ||
          format == DiskType::StreamedCompressed) {
        file.options |= StorageOptions::Streamed;
        stream_index[file.path] = data.location + location;
      } else {
        data_size = read_size(data.data.data(), location);
        file.data = std::span<Byte>(data.data.data() + location, data_size);
      }
    }

    files.push_back(file);
  }
}

auto VirtualDiskReader::process_inline_table() -> void {
  Block& table = blocks[0];
  uint64_t table_pos = 0;

  while (table_pos < table.data.size()) {
    FileData file;

    SizeBlock name_size = read_size(table.data.data(), table_pos);
    file.path = {(char*)table.data.data() + table_pos, name_size};
    table_pos += name_size;
    file.options = read_block<Byte>(table.data.data(), table_pos);

    if (file.options == StorageOptions::Virtualized) {
      SizeBlock data_size = read_size(table.data.data(), table_pos);
      file.data = std::span<Byte>(table.data.data() + table_pos, data_size);
      table_pos += data_size;
    }

    files.push_back(file);
  }
}

auto VirtualDiskReader::dump_info() const -> std::string {
  std::stringstream info_stream;
  info_stream << "Disk Info" << std::endl;
  info_stream << "@type: ";
  switch (get_format()) {
    case DiskType::Standard:
      info_stream << "Standard";
      break;
    case DiskType::Compressed:
      info_stream << "Standard (Compressed)";
      break;
    case DiskType::Streamed:
      info_stream << "Streamed";
      break;
    case DiskType::StreamedCompressed:
      info_stream << "Streamed (Compressed)";
      break;
    case DiskType::Memory:
      info_stream << "Memory (Compressed)";
      break;
  }
  info_stream << std::endl << std::endl;

  info_stream << "@layout: " << std::endl;
  for (const auto& block : get_blocks()) {
    info_stream << " - Block @[" << std::hex << block.location
                << "] size:" << block.data.size() << std::endl;
  }

  if (!get_files().empty()) {
    info_stream << std::endl;
    info_stream << "@files: " << std::endl;

    size_t longest_name = sizeof("[Path]");
    for (const auto& file : get_files()) {
      longest_name = std::max(longest_name, file.path.size());
    }

    info_stream << ">> " << std::setw(longest_name) << std::left << "[Path]";
    info_stream << "  " << std::setw(16) << std::left << "[Size Bytes]";
    info_stream << "  " << "[Flags]";
    info_stream << std::endl;
    for (const auto& file : get_files()) {
      info_stream << "   " << std::setw(longest_name) << std::left << file.path;
      if (file.data.size() > 0) {
        info_stream << "  " << std::setw(16) << std::right << file.data.size();
      } else if (file.options == StorageOptions::Streamed) {
        info_stream << "  " << std::setw(16) << std::right << "Stream";
      } else if (file.options != StorageOptions::Virtualized) {
        info_stream << "  " << std::setw(16) << std::right << "Disk";
      }
      info_stream << "  " << std::setw(8) << std::right
                  << std::bitset<sizeof(file.options.raw()) * 8>(
                         file.options.raw());
      info_stream << std::endl;
    }
  }

  info_stream << std::endl;

  return info_stream.str();
}