// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

constexpr Count checksum_size = sizeof(Bits_32);

// The various blocks that data blobs can be stored outs.
enum class BlockType : Bits_8 {
  Stored = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
};
constexpr Count max_stored_block_size = 65535;

// Validates the zlib header for a blob
static auto validate_header(View::Bytes source) -> Bool {
  constexpr Count zlib_min_input_size = 7;
  constexpr Bits_8 deflate_compression_method = 8;  // Always 8
  constexpr Bits_8 cm_nibble_mask = 0x0F;
  constexpr Bits_32 zlib_fcheck_modulus = 31;
  constexpr Bits_8 zlib_fdict_flag = 0x20;

  // Not enough data to even be a zlib header + blob
  if (source.get_size() < zlib_min_input_size) [[unlikely]] {
    return False;
  }

  // Read out and validate the header.
  Bits_8 zlib_cmf = source[0];
  Bits_8 zlib_flg = source[1];
  if ((zlib_cmf & cm_nibble_mask) != deflate_compression_method) [[unlikely]] {
    return False;
  }
  if (((Bits_32(zlib_cmf) * 256 + zlib_flg) % zlib_fcheck_modulus) != 0)
      [[unlikely]] {
    return False;
  }
  if (zlib_flg & zlib_fdict_flag) [[unlikely]] {
    return False;
  }

  return True;
}

static auto write_header(View::Bytes source, Dynamic::Bytes& output) -> Count {
  constexpr Bits_8 zlib_cmf_byte = 0x78;
  constexpr Bits_8 zlib_flg_byte = 0x9C;
  constexpr Count zlib_header_size = 2;
  constexpr Count stored_block_header_size = 5;
  constexpr Count checksum_size = 4;

  // Caculate the number of blocks based on the max block size.
  const Count block_count = source.get_size() / max_stored_block_size + 1;
  const Count output_size = zlib_header_size +
                            block_count * stored_block_header_size +
                            source.get_size() + checksum_size;

  output.forgetful_resize(output_size);
  auto header_data = output.get_access().get_data();
  Count write_position = 0;

  /// Write the header into the blob
  header_data[write_position++] = zlib_cmf_byte;
  header_data[write_position++] = zlib_flg_byte;

  return write_position;
}

// Caculates adler32 checksum for an arbitrary view.
static auto caculate_checksum(View::Bytes data) -> Bits_32 {
  constexpr Bits_32 adler_modulus = 65521;
  Bits_32 running_sum = 1;
  Bits_32 running_pair_sum = 0;
  for (Count byte_index = 0; byte_index < data.get_size(); byte_index++) {
    running_sum = (running_sum + data[byte_index]) % adler_modulus;
    running_pair_sum = (running_pair_sum + running_sum) % adler_modulus;
  }
  return (running_pair_sum << 16) | running_sum;
}

// Specialized bit level reader since the Reader::Binary works at the byte level
// Bytes consumed LSB-first while Huffman codes accumulated MSB-first so
// canonical decode works without bit-reversal.
struct BitReader {
  View::Bytes data;
  Count bit_position = 0;

  // Converts a bit position to a byte position and then reads a single bit by
  // applying a mask.
  auto read_bit() -> Bool {
    constexpr Static::Bytes<8> bit_masks = {
      0b00000001, 0b00000010, 0b00000100, 0b00001000,
      0b00010000, 0b00100000, 0b01000000, 0b10000000,
    };

    auto logical_bit = bit_position++;
    auto source_byte = data[logical_bit / 8];
    return (source_byte & bit_masks[logical_bit & 0x7]) != 0;
  }

  // Reads up to 32 bits
  // A bit slow and could be optimized for 8 bit blocks but for now this is a
  // quick generalization.
  auto read_bits(Count count) -> Bits_32 {
    Bits_32 result = 0;
    for (Count bit_index = 0; bit_index < count; bit_index++) {
      result |= Bits_32(read_bit().value) << bit_index;
    }
    return result;
  }

  auto read_raw_bytes(Count count) -> View::Bytes {
    // Align to the current byte position
    bit_position = (bit_position + 7) & ~Count(0x7);

    if (bit_position / 8 + count > data.get_size()) [[unlikely]] {
      invalidate();
      return View::Bytes();
    }

    auto result = data.slice(bit_position / 8, count);
    bit_position += count * 8;
    return result;
  }

  auto is_valid() -> Bool { return bit_position <= data.get_size() * 8; }
  auto invalidate() -> void { bit_position = data.get_size() * 8 + 1; }
};

// An allocation free Huffman decoder table.
// Should work for all cases given the constraints of RFC 1951.
struct HuffmanTable {
  // Max code length for literal/length and distance alphabets per RFC 1951.
  static constexpr Count max_code_bits = 15;
  // 288 literal/length symbols + 32 spare slots for distance codes.
  static constexpr Count max_symbol_count = 320;
  static constexpr Bits_16 invalid_symbol = 0xFFFF;

  Static::Vector<Bits_32, max_code_bits + 2> base_code;
  Static::Vector<Count, max_code_bits + 2> base_index;
  Static::Vector<Bits_16, max_symbol_count> symbol_map;
  Count max_bits = 0;

  constexpr HuffmanTable(View::Vector<Bits_8> code_lengths) {
    Static::Vector<Count, HuffmanTable::max_code_bits + 2> bit_length_count;
    for (Count symbol = 0; symbol < code_lengths.get_size(); symbol++) {
      if (code_lengths[symbol] > 0) {
        bit_length_count[code_lengths[symbol]]++;
        if (code_lengths[symbol] > max_bits) {
          max_bits = code_lengths[symbol];
        }
      }
    }

    Bits_32 canonical_code = 0;
    for (Count i = 1; i <= max_bits; i++) {
      base_code[i] = canonical_code;
      canonical_code = (canonical_code + bit_length_count[i]) << 1;
    }

    Count symbol_index = 0;
    for (Count i = 1; i <= max_bits + 1; i++) {
      base_index[i] = symbol_index;
      symbol_index += bit_length_count[i];
    }

    Static::Vector<Count, HuffmanTable::max_code_bits + 2> fill_offset;
    for (Count i = 1; i <= max_bits; i++) {
      fill_offset[i] = base_index[i];
    }

    for (Count symbol = 0; symbol < code_lengths.get_size(); symbol++) {
      Count code_length = code_lengths[symbol];
      if (code_length > 0) {
        symbol_map[fill_offset[code_length]++] = Bits_16(symbol);
      }
    }
  }

  // Looks up a symbol at the current BitReader location.
  // Invalidates the reader if the symbol can't be found.
  constexpr auto decode_symbol(BitReader& reader) const -> Bits_16 {
    Bits_32 accumulated_code = 0;

    // Decodes a symbol in huffman table bit by bit
    for (Count i = 1; i <= max_bits; i++) {
      accumulated_code = (accumulated_code << 1) | reader.read_bit().value;
      const auto code_index = base_index[i];
      const auto base = base_code[i];
      Count symbol_count = base_index[i + 1] - code_index;
      if (symbol_count > 0 && accumulated_code >= base &&
          (accumulated_code - base) < symbol_count) {
        return symbol_map[code_index + (accumulated_code - base)];
      }
    }

    // Couldn't find the symbol so invalidate the reader and exit.
    reader.invalidate();
    return invalid_symbol;
  }
};

constexpr auto make_fixed_literal_table() -> HuffmanTable {
  // Fixed literal/length code lengths per RFC 1951 §3.2.6.
  Static::Vector<Bits_8, 288> code_lengths;

  // Set all of the ranges to the correct value.
  // TODO: Data::set needs a constexpr version.
  for (Count symbol = 0; symbol < 288; symbol++) {
    switch (symbol) {
    case 0 ... 143:
    case 280 ... 287:
      code_lengths[symbol] = 8;
      break;
    case 144 ... 255:
      code_lengths[symbol] = 9;
      break;
    case 256 ... 279:
      code_lengths[symbol] = 7;
      break;
    }
  }

  return HuffmanTable(code_lengths.get_view());
}

constexpr auto make_fixed_distance_table() -> HuffmanTable {
  // All 30 fixed distance codes use length 5 per RFC 1951 §3.2.6.
  Static::Vector<Bits_8, 30> code_lengths;
  for (Count symbol = 0; symbol < 30; symbol++) {
    code_lengths[symbol] = 5;
  }

  return HuffmanTable(code_lengths.get_view());
}

// Length codes 257–285: base lengths and extra bits per RFC 1951 §3.2.5.
constexpr Static::Vector<Bits_16, 29> length_base = {
  3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
  31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

constexpr Static::Vector<Bits_8, 29> length_extra_bits = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
  2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

// Distance codes 0–29: base distances and extra bits per RFC 1951 §3.2.5.
constexpr Static::Vector<Bits_16, 30> distance_base = {
  1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
  33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

constexpr Static::Vector<Bits_8, 30> distance_extra_bits = {
  0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static auto inflate_symbols(
    const HuffmanTable& literal_table,
    const HuffmanTable& distance_table,
    BitReader& reader,
    Dynamic::Bytes& output) -> Bool {
  constexpr Bits_16 end_of_block_symbol = 256;
  constexpr Bits_16 length_code_start = 257;

  while (reader.is_valid()) {
    Bits_16 symbol = literal_table.decode_symbol(reader);
    if (symbol == HuffmanTable::invalid_symbol) [[unlikely]] {
      return False;
    }

    // If we aren't at the end of a block then add it to the output.
    if (symbol < end_of_block_symbol) {
      output.append(Byte(symbol));
      continue;
    }

    // If we are at the end of the the block then exit successfully.
    if (symbol == end_of_block_symbol) {
      return True;
    }

    // Otherwise we got a possible length encoded value that we need to decode.
    // Check if the length code is outside the length code start index.
    // This filters out any invalid symbols.
    Count length_code = symbol - length_code_start;
    if (length_code >= length_base.get_size()) [[unlikely]] {
      return False;
    }
    Count match_length = length_base[length_code] +
                         reader.read_bits(length_extra_bits[length_code]);

    // Look up the distance table value and make sure it matches.
    Bits_16 distance_symbol = distance_table.decode_symbol(reader);
    if (distance_symbol >= distance_base.get_size()) [[unlikely]] {
      return False;
    }

    // Check the match offset which should point to an earlier part of the
    // output.
    Count match_offset = distance_base[distance_symbol] +
                         reader.read_bits(distance_extra_bits[distance_symbol]);
    if (match_offset > output.get_size()) [[unlikely]] {
      return False;
    }

    // Copy the match into the end of the buffer.
    // Since the matches are gurenteed to be non-overlapping we can do a fast
    // memory copy to speed up the operation.
    Count write_start = output.get_size();
    Count copy_start = write_start - match_offset;

    output.resize(output.get_size() + match_length);
    auto bytes = output.get_access().get_data();
    for (Count i = 0; i < match_length; i += match_offset) {
      Data::copy(
          bytes + write_start + i, bytes + copy_start,
          Math::min(match_offset, match_length - i));
    }
  }

  // We ran out of bits before finding the end of block symbol.
  return False;
}

// Inflates a block stored directly in the blob.
static auto inflate_stored(BitReader& reader, Dynamic::Bytes& output) -> Bool {
  constexpr Count stored_block_header_size = 4;
  constexpr Bits_16 stored_block_complement_check = 0xFFFF;

  // Read the header bytes which contain the actual block size.
  auto block_header = reader.read_raw_bytes(stored_block_header_size);
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  // Get the block length and the complement and make sure the match.
  Bits_16 block_length =
      Data::ensure_endian<Data::ByteOrder::Little, Data::ByteOrder::Native>(
          *Data::cast<Bits_16>(block_header.get_data() + 0));
  Bits_16 block_length_complement =
      Data::ensure_endian<Data::ByteOrder::Little, Data::ByteOrder::Native>(
          *Data::cast<Bits_16>(block_header.get_data() + 2));
  if (Bits_16(block_length ^ block_length_complement) !=
      stored_block_complement_check) [[unlikely]] {
    return False;
  }

  // Read bytes equal to the block length if possible.
  auto block_data = reader.read_raw_bytes(block_length);
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  // Successfully read the data so add it to the output block.
  output.concat(block_data);
  return True;
}

// Inflates a fix encoded blob.
static auto inflate_fixed(BitReader& reader, Dynamic::Bytes& output) -> Bool {
  constexpr HuffmanTable literal_table = make_fixed_literal_table();
  constexpr HuffmanTable distance_table = make_fixed_distance_table();
  return inflate_symbols(literal_table, distance_table, reader, output);
}

// Inflates a dynamically encoded blob that requires us to build a huffman table
// from data encoded in the blob.
static auto inflate_dynamic(BitReader& reader, Dynamic::Bytes& output) -> Bool {
  // Special code-length encoding symbols.
  enum class Encodings : Bits_16 {
    Standard = 15,
    Repeat = 16,
    ShortZeros = 17,
    LongZeros = 18,
  };

  // HLIT, HDIST, HCLEN are stored offset from their minimum values.
  constexpr Count hlit_base = 257;
  constexpr Count hdist_base = 1;
  constexpr Count hclen_base = 4;
  Count literal_code_count = reader.read_bits(5) + hlit_base;
  Count distance_code_count = reader.read_bits(5) + hdist_base;
  Count huffman_code_count = reader.read_bits(4) + hclen_base;
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  // Code lengths for the Huffman table are read in this non-sequential order.
  constexpr Count code_length_alphabet_size = 19;
  static constexpr Static::Vector<Bits_8, code_length_alphabet_size>
      code_length_alphabet_order = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                    11, 4,  12, 3, 13, 2, 14, 1, 15};

  // Process the code lengths and then generate a huffman table from the
  // results. Each code is a 3 bit read.
  Static::Vector<Bits_8, code_length_alphabet_size> huffman_lengths;
  for (Count i = 0; i < huffman_code_count; i++) {
    huffman_lengths[code_length_alphabet_order[i]] =
        Bits_8(reader.read_bits(3));
  }

  // Create the dynamic table to decode the symbols for the other tables.
  const auto dynamic_table = HuffmanTable(huffman_lengths);
  Count total_codes = literal_code_count + distance_code_count;
  Static::Vector<Bits_8, HuffmanTable::max_symbol_count> all_lengths;

  Count decode_index = 0;
  while (decode_index < total_codes && reader.is_valid()) {
    Bits_16 symbol = dynamic_table.decode_symbol(reader);

    // Minimum run lengths for each special code.
    constexpr Count repeat_min = 3;
    constexpr Count short_zeros_min = 3;
    constexpr Count long_zeros_min = 11;

    // Each Encoding ecodes a value and a number of elements that should be
    // inserted.
    Bits_8 value;
    Count element_count;
    switch (symbol) {
    case 0 ... Bits_16(Encodings::Standard):
      value = Bits_8(symbol);
      element_count = 1;
      break;
    case Bits_16(Encodings::Repeat):
      value = decode_index > 0 ? all_lengths[decode_index - 1] : 0;
      element_count = reader.read_bits(2) + repeat_min;
      break;
    case Bits_16(Encodings::ShortZeros):
      value = 0;
      element_count = reader.read_bits(3) + short_zeros_min;
      break;
    case Bits_16(Encodings::LongZeros):
      value = 0;
      element_count = reader.read_bits(7) + long_zeros_min;
      break;
    }

    // Check if more elements were requested than the number of expected codes.
    if (decode_index + element_count > total_codes) {
      Diagnostics::Log::error(
          "Compression: dynamic block code-length sequence overruns total code count"_view);
      return False;
    }

    // Set the range to the expected value.
    Data::set(all_lengths.get_data() + decode_index, value, element_count);
    decode_index += element_count;
  }

  // If the reader is invalid then we failed to decode all of the lengths.
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  // Split the range into the two actual tables
  const auto literal_symbols =
      all_lengths.get_view().slice(0, literal_code_count);
  const auto distance_symbols =
      all_lengths.get_view().slice(literal_code_count);

  // Inflate using the extracted literal and distance tables.
  return inflate_symbols(
      HuffmanTable(literal_symbols), HuffmanTable(distance_symbols), reader,
      output);
}

auto Compression::inflate(const View::Bytes source) -> Dynamic::Bytes {
  // If we can't validate the type passed to us then we can just return an empty
  // blob since we most likely won't be able to process the data anyway.
  if (!validate_header(source)) {
    return Dynamic::Bytes();
  }

  // Create a bit reader over the source accounting for the zlib envelope.
  constexpr Count zlib_envelope_overhead = 6;
  BitReader reader{source.slice(2, source.get_size() - zlib_envelope_overhead)};
  Dynamic::Bytes output;
  output.ensure_capacity(source.get_size() * 4);

  // Read out all the blocks until we find the end bit.
  Bool final_block = false;
  while (reader.is_valid() && !final_block) {
    // If we start off by reading 1 then we are in the final block.
    final_block = reader.read_bits(1);

    // Detect the actual block type
    auto block_type = BlockType(reader.read_bits(2));
    switch (block_type) {
    case BlockType::Stored:
      if (!inflate_stored(reader, output)) {
        Diagnostics::Log::error("Compression: stored block decompression failed"_view);
        return Dynamic::Bytes();
      }
      break;
    case BlockType::FixedHuffman:
      if (!inflate_fixed(reader, output)) {
        Diagnostics::Log::error("Compression: fixed Huffman block decompression failed"_view);
        return Dynamic::Bytes();
      }
      break;
    case BlockType::DynamicHuffman:
      if (!inflate_dynamic(reader, output)) {
        Diagnostics::Log::error("Compression: dynamic Huffman block decompression failed"_view);
        return Dynamic::Bytes();
      }
      break;
    default:
      Diagnostics::Log::error("Compression: unknown block type in DEFLATE stream"_view);
      return Dynamic::Bytes();
    }
  }

  if (!reader.is_valid()) [[unlikely]] {
    return Dynamic::Bytes();
  }

  // Validate the final checksum and return nothing if the data is corrupted.
  Bits_32 stored_checksum =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(
              source.get_data() + source.get_size() - checksum_size));
  if (caculate_checksum(output.get_view()) != stored_checksum) [[unlikely]] {
    Diagnostics::Log::error("Compression: Adler-32 checksum mismatch"_view);
    return Dynamic::Bytes();
  }

  return output;
}

auto Compression::deflate(const View::Bytes source) -> Dynamic::Bytes {
  Dynamic::Bytes output;
  Count write_position = write_header(source, output);
  auto blob_data = output.get_access().get_data();

  // Write out all of the blocksd as raw data since we currently don't do
  // any actual compression.
  for (Count source_offset = 0; source_offset <= source.get_size();) {
    const Count block_size =
        Math::min(source.get_size() - source_offset, max_stored_block_size);

    // Check if this is the final block we need to process.
    const Bits_8 is_final_block =
        (source_offset + block_size >= source.get_size()) ? 0x01 : 0x00;

    // Header stores if this is the final block and the 16 bit size in two
    // forms for validation.
    blob_data[write_position++] = is_final_block;
    blob_data[write_position++] = Byte(block_size & 0xFF);
    blob_data[write_position++] = Byte((block_size >> 8) & 0xFF);
    blob_data[write_position++] = Byte((~block_size) & 0xFF);
    blob_data[write_position++] = Byte(((~block_size) >> 8) & 0xFF);

    // Block copy the source data.
    if (block_size > 0) {
      Data::copy(
          blob_data + write_position, source.get_data() + source_offset,
          block_size);
      write_position += block_size;
    }

    // If this was the final block then terminate.
    if (is_final_block) {
      break;
    }

    // Increment source offset
    source_offset += block_size;
  }

  // Store the final checksum
  Bits_32 checksum = caculate_checksum(source);
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(blob_data + write_position), checksum);
  write_position += checksum_size;

  // Shrink the output down to the correct size.
  output.resize(write_position);
  return output;
}
