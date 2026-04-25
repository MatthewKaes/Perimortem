// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/archive/reader.hpp"

#include <zstd.h>

using namespace Perimortem::Memory;
using namespace Perimortem::Serialization::Archive;

auto read_size(const Core::View::Amorphous source, SizeBlock& pos) -> SizeBlock {
  // Read the run length size encoding byte
  Byte size_bytes = source.incremental_read<Byte>(pos);

  switch (size_bytes) {
    case sizeof(Bits_8):
      return source.incremental_read<Bits_8>(pos);
    case sizeof(Bits_16):
      return source.incremental_read<Bits_16>(pos);
    case sizeof(Bits_32):
      return source.incremental_read<Bits_32>(pos);
    case sizeof(Bits_64):
      return source.incremental_read<Bits_64>(pos);
  }

  // TODO: Error handling.
  __builtin_debugtrap();
  return -1;
}

auto Reader::Load(Core::View::Amorphous archive) {
  blocks[0].reset();
  blocks[1].reset();


  Count read_cursor = 0;
  auto header = archive.incremental_read<HeaderBlock>(read_cursor);
  if (header != autogenetic_header) {
    // TODO: Error handling
    __builtin_debugtrap();
    return;
  }

  // Allocate the required number of data blocks.
  format = archive.incremental_read<Type>(read_cursor);
  Count block_count;
  switch (format) {
    // Memroy speed reads the data from a single continous buffer.
    case Type::Memory:
      block_count = 1;
      break;

    default:
      block_count = 2;
  }

  for (int i = 0; i < block_count; i++) {
    SizeBlock block_size = archive.incremental_read<SizeBlock>(read_cursor);

    auto& block_data = blocks[i];

    // Load non-empty blocks.
    if (block_size > 0) {
      block_data.ensure_capacity(block_size);

      // For streamed blocks don't read anything beyond the table.
      if (i > 0 && (reader->format == Type::Streamed ||
                    reader->format == Type::StreamedCompressed)) {
        disk_reader.sync(block_size);
      } else {
        // Spicy const cast here but we know we explicitly own the arena data.
        if (!disk_reader.load(
                const_cast<char*>(block_data.data.get_view().get_data()),
                block_size)) {
          /* TODO: Error handling. */
          __builtin_debugtrap();
          return nullptr;
        }
      }

      // Decompress blocks.
      switch (reader->format) {
        case Type::Compressed:
        case Type::Memory:
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
    case Type::Streamed:
    case Type::StreamedCompressed:
    case Type::Standard:
    case Type::Compressed:
      reader->process_split_table();
      break;

    // Inline headers just need to write the size
    case Type::Memory:
      reader->process_inline_table();
      break;
  }

  return reader;
}

auto Reader::stream_from_disk(const Core::View::Amorphous path, Managed::Bytes& data)
    -> Bool {
  if (!stream_index.contains(path))
    return false;

  if (!std::filesystem::exists(disk_path))
    return false;

  filesystem_reader disk_reader(disk_path);
  disk_reader.sync(stream_index[path]);

  Byte block_size_buffer[max_run_length_bytes];
  Count run_length = 0;
  if (!disk_reader.load_object(block_size_buffer)) {
    // TODO: Error handling
    __builtin_debugtrap();
    return false;
  }
  SizeBlock block_size = read_size(
      Core::View::Amorphous(block_size_buffer, max_run_length_bytes), run_length);

  // Adjust to the correct position so we are at the start of the buffer.
  disk_reader.sync(run_length - max_run_length_bytes);

  data.reset(block_size);
  if (!disk_reader.load(const_cast<char*>(data.get_view().get_data()),
                        block_size)) {
    // TODO: Error handling
    __builtin_debugtrap();
    return false;
  }

  // Decompress the streamed data.
  if (format == Type::StreamedCompressed) {
    decompress_block(data);
  }

  return true;
}

auto Reader::decompress_block(Managed::Bytes& data) -> void {
  static auto dctx = ZSTD_createDCtx();
  if (dctx == nullptr) {
    // TODO: Error handling.
    __builtin_debugtrap();
  }

  Count inflated_size =
      ZSTD_getFrameContentSize(data.get_view().get_data(), data.get_size());
  data.get_arena().allocate(inflated_size);
  Managed::Bytes output(data.get_arena());
  output.reset(inflated_size);

  ZSTD_decompressDCtx(dctx, const_cast<char*>(output.get_view().get_data()),
                      output.get_size(), data.get_view().get_data(),
                      data.get_size());

  // Swap data to the new buffer and then hand off to the next layer.
  data.take(output);
}

auto Reader::process_split_table() -> void {
  Block& header_block = blocks[0];
  Block& data_block = blocks[1];
  Count table_pos = 0;
  Core::View::Amorphous header_view = header_block.data.get_view();
  Core::View::Amorphous data_view = data_block.data.get_view();

  while (table_pos < header_view.get_size()) {
    FileData file;

    SizeBlock name_size = read_size(header_view, table_pos);
    file.path = header_view.slice(table_pos, name_size);
    table_pos += name_size;
    file.options = header_view.incremental_read<Byte>(table_pos);

    if (file.options == FileStorage::Virtualized) {
      SizeBlock location = read_size(header_view, table_pos);

      SizeBlock data_size = -1;
      // Stream disks don't preload any data, even when virtualized.
      if (format == Type::Streamed || format == Type::StreamedCompressed) {
        file.options |= FileStorage::Streamed;
        stream_index[file.path] = data_block.location + location;
      } else {
        data_size = read_size(data_block.data.get_view(), location);
        file.data = data_view.slice(location, data_size);
      }
    }

    files.insert(file);
  }
}

auto Reader::process_inline_table() -> void {
  Block& full_block = blocks[0];
  Count table_pos = 0;
  Core::View::Amorphous full_block_view = full_block.data.get_view();

  while (table_pos < full_block_view.get_size()) {
    FileData file;

    SizeBlock name_size = read_size(full_block_view, table_pos);
    file.path = full_block_view.slice(table_pos, name_size);
    table_pos += name_size;
    file.options = full_block_view.incremental_read<Byte>(table_pos);

    if (file.options == FileStorage::Virtualized) {
      SizeBlock data_size = read_size(full_block_view, table_pos);
      file.data = full_block_view.slice(table_pos, data_size);
      table_pos += data_size;
    }

    files.insert(file);
  }
}

auto Reader::dump_info() const -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes info_stream;
  info_stream.append("Disk Info\n"_view);
  info_stream << "Disk Info" << std::endl;
  info_stream << "@type: ";
  switch (get_format()) {
    case Type::Standard:
      info_stream << "Standard";
      break;
    case Type::Compressed:
      info_stream << "Standard (Compressed)";
      break;
    case Type::Streamed:
      info_stream << "Streamed";
      break;
    case Type::StreamedCompressed:
      info_stream << "Streamed (Compressed)";
      break;
    case Type::Memory:
      info_stream << "Memory (Compressed)";
      break;
  }
  info_stream << std::endl << std::endl;

  info_stream << "@layout: " << std::endl;
  const auto blocks = get_blocks();
  for (Count i = 0; i < blocks.get_size(); i++) {
    info_stream << " - Block @[" << std::hex << blocks[i].location
                << "] size:" << blocks[i].data.get_size() << std::endl;
  }

  if (!get_files().empty()) {
    info_stream << std::endl;
    info_stream << "@files: " << std::endl;

    Count longest_name = sizeof("[Path]");
    const auto files = get_files();
    for (Count i = 0; i < files.get_size(); i++) {
      longest_name = std::max(longest_name, files[i].path.get_size());
    }

    info_stream << ">> " << std::setw(longest_name) << std::left << "[Path]";
    info_stream << "  " << std::setw(16) << std::left << "[Size Bytes]";
    info_stream << "  " << "[Flags]";
    info_stream << std::endl;
    for (Count i = 0; i < files.get_size(); i++) {
      info_stream << "   " << std::setw(longest_name) << std::left
                  << files[i].path;
      if (files[i].data.get_size() > 0) {
        info_stream << "  " << std::setw(16) << std::right
                    << file.data.get_size();
      } else if (files[i].options == FileStorage::Streamed) {
        info_stream << "  " << std::setw(16) << std::right << "Stream";
      } else if (files[i].options != FileStorage::Virtualized) {
        info_stream << "  " << std::setw(16) << std::right << "Disk";
      }
      info_stream << "  " << std::setw(8) << std::right
                  << std::bitset<sizeof(files[i].options.raw()) * 8>(
                         files[i].options.raw());
      info_stream << std::endl;
    }
  }

  info_stream << std::endl;

  return info_stream.str();
}