// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/deflate.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/system/compression/bit_stream/reader.hpp"
#include "perimortem/system/compression/bit_stream/writer.hpp"
#include "perimortem/system/compression/huffman.hpp"
#include "perimortem/system/compression/lz77.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

namespace Perimortem::System::Compression {

enum class BlockType : Bits_8 {
  Stored = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
};

constexpr Count max_stored_block_size = 1 << 16;

static auto validate_header(View::Bytes source) -> Bool {
  constexpr Count deflate_min_input_size = 7;
  constexpr Bits_8 deflate_compression_method = 8;
  constexpr Bits_8 cm_nibble_mask = 0x0F;
  constexpr Bits_32 deflate_fcheck_modulus = 31;
  constexpr Bits_8 deflate_fdict_flag = 0x20;

  if (source.get_size() < deflate_min_input_size) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: Input too short to be a valid deflate stream"_view);
    return False;
  }

  Bits_8 deflate_cmf = source[0];
  Bits_8 deflate_flg = source[1];
  if ((deflate_cmf & cm_nibble_mask) != deflate_compression_method)
      [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: Unsupported compression method in deflate header"_view);
    return False;
  }
  if (((Bits_32(deflate_cmf) * 256 + deflate_flg) % deflate_fcheck_modulus) !=
      0) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: deflate header integrity check failed"_view);
    return False;
  }
  if (deflate_flg & deflate_fdict_flag) [[unlikely]] {
    Diagnostics::Log::error(
        "Compression: preset dictionary is not supported"_view);
    return False;
  }

  return True;
}

static auto write_header(Dynamic::Bytes& output) -> void {
  constexpr Bits_8 deflate_cmf_byte = 0x78;
  constexpr Bits_8 deflate_flg_byte = 0x9C;
  output.append(Byte(deflate_cmf_byte));
  output.append(Byte(deflate_flg_byte));
}

// Adler-32 per RFC 1950 §8.2.
static auto calculate_checksum(View::Bytes data) -> Bits_32 {
  constexpr Bits_32 adler_modulus = 65521;
  Bits_32 running_sum = 1;
  Bits_32 running_pair_sum = 0;

  for (Count i = 0; i < data.get_size(); i++) {
    running_sum = (running_sum + data[i]) % adler_modulus;
    running_pair_sum = (running_pair_sum + running_sum) % adler_modulus;
  }

  return (running_pair_sum << 16) | running_sum;
}

// Length codes 257-285: base lengths and extra bits per RFC 1951 §3.2.5.
constexpr Static::Vector<Bits_16, 29> length_base = {
  3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
  31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

constexpr Static::Vector<Bits_8, 29> length_extra_bits = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
  2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

// Distance codes 0-29: base distances and extra bits per RFC 1951 §3.2.5.
constexpr Static::Vector<Bits_16, 30> distance_base = {
  1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
  33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

constexpr Static::Vector<Bits_8, 30> distance_extra_bits = {
  0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

constexpr Count code_length_count = 19;

// Code-length alphabet ordering per RFC 1951 §3.2.7. The order puts the most
// commonly non-zero lengths first so the transmitted sequence can be truncated
// as soon as all trailing entries are zero.
constexpr Static::Vector<Bits_8, code_length_count> code_length_order = {
  16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

// A length or distance value mapped to its DEFLATE back-reference encoding:
// a base symbol index into the literal/length or distance alphabet, plus an
// extra-bits field that encodes the remainder per RFC 1951 §3.2.5.
struct BackReferenceEncoding {
  BackReferenceEncoding() : symbol(0), extra_bits(0), extra_value(0) {}
  BackReferenceEncoding(Count symbol, Bits_32 extra_value, Count extra_bits)
      : symbol(symbol), extra_bits(extra_bits), extra_value(extra_value) {}

  auto get_symbol() const -> Count { return symbol; }
  auto get_extra_value() const -> Bits_32 { return extra_value; }
  auto get_extra_bits() const -> Count { return extra_bits; }

 private:
  Count symbol;
  Count extra_bits;
  Bits_32 extra_value;
};

static auto encode_back_reference(
    Count value,
    View::Vector<Bits_16> base_table,
    View::Vector<Bits_8> extra_bits_table,
    Count symbol_offset) -> BackReferenceEncoding {
  for (Count i = base_table.get_size() - 1; i > 0; i--) {
    if (value >= base_table[i]) {
      return BackReferenceEncoding(
          symbol_offset + i, Bits_32(value - base_table[i]),
          extra_bits_table[i]);
    }
  }
  return BackReferenceEncoding(symbol_offset, 0, 0);
}

static auto inflate_symbols(
    const HuffmanTable& literal_table,
    const HuffmanTable& distance_table,
    BitStream::Reader& reader,
    Dynamic::Bytes& output) -> Bool {
  constexpr Bits_16 end_of_block_symbol = 256;
  constexpr Bits_16 length_code_start = 257;

  while (reader.is_valid()) {
    Bits_16 symbol = literal_table.decode_symbol(reader);
    if (symbol == HuffmanTable::invalid_symbol) [[unlikely]] {
      return False;
    }

    if (symbol < end_of_block_symbol) {
      output.append(Byte(symbol));
      continue;
    }

    if (symbol == end_of_block_symbol) {
      return True;
    }

    Count length_code = symbol - length_code_start;
    if (length_code >= length_base.get_size()) [[unlikely]] {
      return False;
    }
    Count match_length = length_base[length_code] +
                         reader.read_code(length_extra_bits[length_code]);

    Bits_16 distance_symbol = distance_table.decode_symbol(reader);
    if (distance_symbol >= distance_base.get_size()) [[unlikely]] {
      return False;
    }

    Count match_offset = distance_base[distance_symbol] +
                         reader.read_code(distance_extra_bits[distance_symbol]);
    if (match_offset > output.get_size()) [[unlikely]] {
      return False;
    }

    // Non-overlapping back-references can be bulk-copied in strides.
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

  return False;
}

static auto inflate_stored(BitStream::Reader& reader, Dynamic::Bytes& output)
    -> Bool {
  constexpr Count stored_block_header_size = 4;
  constexpr Bits_16 stored_block_complement_check = 0xFFFF;

  auto block_header = reader.read_raw_bytes(stored_block_header_size);
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

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

  auto block_data = reader.read_raw_bytes(block_length);
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  output.concat(block_data);
  return True;
}

static auto inflate_fixed(BitStream::Reader& reader, Dynamic::Bytes& output)
    -> Bool {
  constexpr HuffmanTable literal_table = HuffmanTable::make_fixed_literal();
  constexpr HuffmanTable distance_table = HuffmanTable::make_fixed_distance();
  return inflate_symbols(literal_table, distance_table, reader, output);
}

static auto inflate_dynamic(BitStream::Reader& reader, Dynamic::Bytes& output)
    -> Bool {
  enum class Encodings : Bits_16 {
    Standard = 15,
    Repeat = 16,
    ShortZeros = 17,
    LongZeros = 18,
  };

  constexpr Count hlit_base = 257;
  constexpr Count hdist_base = 1;
  constexpr Count hclen_base = 4;
  Count literal_code_count = reader.read_code(5) + hlit_base;
  Count distance_code_count = reader.read_code(5) + hdist_base;
  Count huffman_code_count = reader.read_code(4) + hclen_base;
  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  Static::Vector<Bits_8, code_length_count> huffman_lengths;
  for (Count i = 0; i < huffman_code_count; i++) {
    huffman_lengths[code_length_order[i]] = Bits_8(reader.read_code(3));
  }

  const auto dynamic_table = HuffmanTable(huffman_lengths);
  Count total_codes = literal_code_count + distance_code_count;
  Static::Vector<Bits_8, HuffmanTable::max_symbol_count> all_lengths;

  Count decode_index = 0;
  while (decode_index < total_codes && reader.is_valid()) {
    Bits_16 symbol = dynamic_table.decode_symbol(reader);

    constexpr Count repeat_min = 3;
    constexpr Count short_zeros_min = 3;
    constexpr Count long_zeros_min = 11;

    Bits_8 value;
    Count element_count;
    switch (symbol) {
    case 0 ... Bits_16(Encodings::Standard):
      value = Bits_8(symbol);
      element_count = 1;
      break;
    case Bits_16(Encodings::Repeat):
      value = decode_index > 0 ? all_lengths[decode_index - 1] : 0;
      element_count = reader.read_code(2) + repeat_min;
      break;
    case Bits_16(Encodings::ShortZeros):
      value = 0;
      element_count = reader.read_code(3) + short_zeros_min;
      break;
    case Bits_16(Encodings::LongZeros):
      value = 0;
      element_count = reader.read_code(7) + long_zeros_min;
      break;
    }

    if (decode_index + element_count > total_codes) {
      Diagnostics::Log::error(
          "Compression: dynamic block code-length sequence overruns total code count"_view);
      return False;
    }

    Data::set(all_lengths.get_data() + decode_index, value, element_count);
    decode_index += element_count;
  }

  if (!reader.is_valid()) [[unlikely]] {
    return False;
  }

  const auto literal_symbols =
      all_lengths.get_view().slice(0, literal_code_count);
  const auto distance_symbols =
      all_lengths.get_view().slice(literal_code_count);

  return inflate_symbols(
      HuffmanTable(literal_symbols), HuffmanTable(distance_symbols), reader,
      output);
}

static auto write_checksum(Dynamic::Bytes& output, View::Bytes source) -> void {
  Bits_32 checksum = calculate_checksum(source);

  output.resize(output.get_size() + sizeof(Bits_32));
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(
          output.get_access().get_data() + output.get_size() - sizeof(Bits_32)),
      checksum);
}

static auto test_checksum(View::Bytes stream, View::Bytes source) -> Bool {
  Bits_32 stream_checksum = calculate_checksum(stream);

  Bits_32 checksum =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(
              source.get_data() + source.get_size() - sizeof(Bits_32)));

  if (stream_checksum != checksum) [[unlikely]] {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message << "Compression: Adler-32 checksum mismatch. checksum="_view
                  << checksum << " stream_checksum="_view << stream_checksum;
    Diagnostics::Log::error(error_message);
    return False;
  }

  return True;
}

static auto write_lz77_block(
    View::Bytes source,
    Count search_depth,
    Lz77& lz77,
    BitStream::Writer& writer,
    const HuffmanTable& lit_table,
    const HuffmanTable& dist_table) -> void {
  Count pos = 0;
  while (pos < source.get_size()) {
    auto match = lz77.find_match(source, pos, search_depth);
    lz77.insert(source, pos);

    if (match.get_length() >= Lz77::min_match) {
      const auto len_enc = encode_back_reference(
          match.get_length(), length_base.get_view(),
          length_extra_bits.get_view(), 257);
      auto literal_code = lit_table.encode_symbol(len_enc.get_symbol());
      writer.write_code(literal_code.get_code(), literal_code.get_length());
      writer.write_bits(len_enc.get_extra_value(), len_enc.get_extra_bits());

      const auto dist_enc = encode_back_reference(
          match.get_distance(), distance_base.get_view(),
          distance_extra_bits.get_view(), 0);
      auto distance_code = dist_table.encode_symbol(dist_enc.get_symbol());
      writer.write_code(distance_code.get_code(), distance_code.get_length());
      writer.write_bits(dist_enc.get_extra_value(), dist_enc.get_extra_bits());
      pos += match.get_length();
    } else {
      auto literal_code = lit_table.encode_symbol(Count(source[pos]));
      writer.write_code(literal_code.get_code(), literal_code.get_length());
      pos++;
    }
  }
}

static auto write_deflate_footer(
    BitStream::Writer& writer,
    const HuffmanTable& lit_table,
    Dynamic::Bytes& output,
    View::Bytes source) -> void {
  constexpr Count end_of_block = 256;
  auto eob_code = lit_table.encode_symbol(end_of_block);

  writer.write_code(eob_code.get_code(), eob_code.get_length());
  writer.flush();
  write_checksum(output, source);
}

// A single entry in a run-length encoded code-length sequence per RFC 1951
// §3.2.7: the symbol to emit (0-18), how many extra bits follow, and the value
// of those extra bits.
struct RleEntry {
  RleEntry() : extra_value(0), symbol(0), extra_bits(0) {}
  RleEntry(Bits_8 symbol, Bits_8 extra_bits, Bits_32 extra_value)
      : extra_value(extra_value), symbol(symbol), extra_bits(extra_bits) {}

  auto get_symbol() const -> Bits_8 { return symbol; }
  auto get_extra_bits() const -> Bits_8 { return extra_bits; }
  auto get_extra_value() const -> Bits_32 { return extra_value; }

 private:
  Bits_32 extra_value;
  Bits_8 symbol;
  Bits_8 extra_bits;
};

static auto rle_encode_code_lengths(
    View::Vector<Bits_8> ll_lengths,
    View::Vector<Bits_8> dist_lengths,
    Static::Vector<RleEntry, 320>& out) -> Count {
  const Count ll_count = ll_lengths.get_size();
  auto lengths_at = [&](Count idx) -> Bits_8 {
    return idx < ll_count ? ll_lengths[idx] : dist_lengths[idx - ll_count];
  };

  const Count total = ll_count + dist_lengths.get_size();
  Count count = 0;
  Count pos = 0;
  while (pos < total) {
    Bits_8 len = lengths_at(pos);
    if (len == 0) {
      Count run = 1;
      while (pos + run < total && lengths_at(pos + run) == 0 && run < 138) {
        run++;
      }
      if (run >= 11) {
        out[count++] = RleEntry(Bits_8(18), Bits_8(7), Bits_32(run - 11));
      } else if (run >= 3) {
        out[count++] = RleEntry(Bits_8(17), Bits_8(3), Bits_32(run - 3));
      } else {
        for (Count r = 0; r < run; r++) {
          out[count++] = RleEntry();
        }
      }
      pos += run;
    } else {
      out[count++] = RleEntry(len, Bits_8(0), Bits_32(0));
      pos++;
      Count run = 0;
      while (pos + run < total && lengths_at(pos + run) == len && run < 6) {
        run++;
      }
      if (run >= 3) {
        out[count++] = RleEntry(Bits_8(16), Bits_8(2), Bits_32(run - 3));
        pos += run;
      }
    }
  }
  return count;
}

static auto write_dynamic_block_header(
    BitStream::Writer& writer,
    View::Vector<Bits_8> ll_lengths,
    Count hlit,
    View::Vector<Bits_8> dist_lengths,
    Count hdist) -> void {
  Static::Vector<RleEntry, 320> rle;
  Count rle_count = rle_encode_code_lengths(
      View::Vector<Bits_8>(ll_lengths.get_data(), hlit + 257),
      View::Vector<Bits_8>(dist_lengths.get_data(), hdist + 1), rle);

  Static::Vector<Bits_32, code_length_count> meta_freq;
  for (Count i = 0; i < code_length_count; i++) {
    meta_freq[i] = 0;
  }
  for (Count i = 0; i < rle_count; i++) {
    meta_freq[rle[i].get_symbol()]++;
  }

  Static::Vector<Bits_8, code_length_count> meta_lengths;
  HuffmanTable::compute_lengths(
      meta_freq.get_view(), meta_lengths.get_access());
  const HuffmanTable meta_table(meta_lengths.get_view());

  // Trim trailing zero-length entries from the meta table. The minimum allowed
  // by RFC 1951 is 4, so we stop at index 3.
  Count hclen = code_length_count - 1;
  while (hclen > 3 && meta_lengths[code_length_order[hclen]] == 0) {
    hclen--;
  }

  writer.write_bits(Bits_32(hlit), 5);
  writer.write_bits(Bits_32(hdist), 5);
  writer.write_bits(Bits_32(hclen - 3), 4);

  for (Count i = 0; i <= hclen; i++) {
    writer.write_bits(meta_lengths[code_length_order[i]], 3);
  }
  for (Count i = 0; i < rle_count; i++) {
    auto meta_code = meta_table.encode_symbol(rle[i].get_symbol());
    writer.write_code(meta_code.get_code(), meta_code.get_length());
    writer.write_bits(rle[i].get_extra_value(), rle[i].get_extra_bits());
  }
}

static auto deflate_stored(View::Bytes source) -> Dynamic::Bytes {
  constexpr Count deflate_header_size = 2;
  constexpr Count stored_block_header_size = 5;
  const Count block_count = source.get_size() / max_stored_block_size + 1;

  const Count output_size = deflate_header_size +
                            block_count * stored_block_header_size +
                            source.get_size() + sizeof(Bits_32);

  Dynamic::Bytes output;
  output.ensure_capacity(output_size);
  write_header(output);

  for (Count i = 0; i <= source.get_size();) {
    const Count block_size =
        Math::min(source.get_size() - i, max_stored_block_size);
    const Bits_8 is_final_block =
        (i + block_size >= source.get_size()) ? 0x01 : 0x00;

    output.append(Byte(is_final_block));
    output.append(Byte(block_size & 0xFF));
    output.append(Byte((block_size >> 8) & 0xFF));
    output.append(Byte((~block_size) & 0xFF));
    output.append(Byte(((~block_size) >> 8) & 0xFF));

    if (block_size > 0) {
      output.concat(source.slice(i, block_size));
    }

    if (is_final_block) {
      break;
    }

    i += block_size;
  }

  write_checksum(output, source);
  return output;
}

static auto deflate_fixed(View::Bytes source, Count search_depth)
    -> Dynamic::Bytes {
  constexpr HuffmanTable lit_table = HuffmanTable::make_fixed_literal();
  constexpr HuffmanTable dist_table = HuffmanTable::make_fixed_distance();

  Dynamic::Bytes output;
  output.ensure_capacity(source.get_size() + 128);
  write_header(output);

  BitStream::Writer writer(output);
  writer.write_bits(0x01, 1);
  writer.write_bits(Bits_32(BlockType::FixedHuffman), 2);

  if (source.get_size() > 0) {
    Lz77 lz77;
    write_lz77_block(source, search_depth, lz77, writer, lit_table, dist_table);
  }

  write_deflate_footer(writer, lit_table, output, source);
  return output;
}

constexpr Count deflate_lit_len_count = 286;
constexpr Count deflate_dist_count = 30;

static auto deflate_dynamic(View::Bytes source, Count search_depth)
    -> Dynamic::Bytes {
  if (source.get_size() == 0) {
    return deflate_fixed(source, search_depth);
  }

  // First pass: walk the LZ77 stream to collect symbol frequencies so we can
  // build optimal Huffman code lengths before writing any output.
  Lz77 lz77;

  Static::Vector<Bits_32, deflate_lit_len_count> ll_freq;
  Static::Vector<Bits_32, deflate_dist_count> dist_freq;
  for (Count i = 0; i < deflate_lit_len_count; i++) {
    ll_freq[i] = 0;
  }
  for (Count i = 0; i < deflate_dist_count; i++) {
    dist_freq[i] = 0;
  }
  ll_freq[256] = 1;

  Count pos = 0;
  while (pos < source.get_size()) {
    auto match = lz77.find_match(source, pos, search_depth);
    lz77.insert(source, pos);
    if (match.get_length() >= Lz77::min_match) {
      ll_freq[encode_back_reference(
                  match.get_length(), length_base.get_view(),
                  length_extra_bits.get_view(), 257)
                  .get_symbol()]++;
      dist_freq[encode_back_reference(
                    match.get_distance(), distance_base.get_view(),
                    distance_extra_bits.get_view(), 0)
                    .get_symbol()]++;
      pos += match.get_length();
    } else {
      ll_freq[Count(source[pos])]++;
      pos++;
    }
  }

  Static::Vector<Bits_8, deflate_lit_len_count> ll_lengths;
  Static::Vector<Bits_8, deflate_dist_count> d_lengths;
  HuffmanTable::compute_lengths(ll_freq.get_view(), ll_lengths.get_access());
  HuffmanTable::compute_lengths(dist_freq.get_view(), d_lengths.get_access());

  if (ll_lengths[256] == 0) {
    ll_lengths[256] = 1;
  }
  Bool any_dist = False;
  for (Count i = 0; i < deflate_dist_count; i++) {
    if (d_lengths[i] > 0) {
      any_dist = True;
      break;
    }
  }
  if (!any_dist.value) {
    d_lengths[0] = 1;
  }

  const HuffmanTable ll_table(ll_lengths.get_view());
  const HuffmanTable dist_table(d_lengths.get_view());

  Count hlit = 0;
  for (Count s = deflate_lit_len_count - 1; s > 256; s--) {
    if (ll_lengths[s] > 0) {
      hlit = s - 256;
      break;
    }
  }
  Count hdist = 0;
  for (Count s = deflate_dist_count - 1; s > 0; s--) {
    if (d_lengths[s] > 0) {
      hdist = s;
      break;
    }
  }

  Dynamic::Bytes output;
  output.ensure_capacity(source.get_size() + 512);
  write_header(output);

  BitStream::Writer writer(output);
  writer.write_bits(0x01, 1);
  writer.write_bits(Bits_32(BlockType::DynamicHuffman), 2);
  write_dynamic_block_header(
      writer, ll_lengths.get_view(), hlit, d_lengths.get_view(), hdist);

  lz77.reset();
  write_lz77_block(source, search_depth, lz77, writer, ll_table, dist_table);

  write_deflate_footer(writer, ll_table, output, source);
  return output;
}

auto inflate(const View::Bytes source) -> Dynamic::Bytes {
  if (!validate_header(source)) {
    return Dynamic::Bytes();
  }

  constexpr Count deflate_envelope_overhead = 6;
  BitStream::Reader reader{
    source.slice(2, source.get_size() - deflate_envelope_overhead)};
  Dynamic::Bytes output;
  output.ensure_capacity(source.get_size() * 4);

  Bool final_block = false;
  while (reader.is_valid() && !final_block) {
    final_block = reader.read_code(1);

    auto block_type = BlockType(reader.read_code(2));
    switch (block_type) {
    case BlockType::Stored:
      if (!inflate_stored(reader, output)) {
        Diagnostics::Log::error(
            "Compression: stored block decompression failed"_view);
        return Dynamic::Bytes();
      }
      break;
    case BlockType::FixedHuffman:
      if (!inflate_fixed(reader, output)) {
        Diagnostics::Log::error(
            "Compression: fixed Huffman block decompression failed"_view);
        return Dynamic::Bytes();
      }
      break;
    case BlockType::DynamicHuffman:
      if (!inflate_dynamic(reader, output)) {
        Diagnostics::Log::error(
            "Compression: dynamic Huffman block decompression failed"_view);
        return Dynamic::Bytes();
      }
      break;
    default:
      Diagnostics::Log::error(
          "Compression: unknown block type in deflate stream"_view);
      return Dynamic::Bytes();
    }
  }

  if (!reader.is_valid()) [[unlikely]] {
    return Dynamic::Bytes();
  }

  if (!test_checksum(output, source)) {
    return Dynamic::Bytes();
  }

  return output;
}

auto deflate(View::Bytes source, Level level) -> Dynamic::Bytes {
  constexpr Count default_search_depth = 32;
  constexpr Count best_search_depth = 128;

  switch (level) {
  case Level::None:
    return deflate_stored(source);
  default: {
    Static::Bytes<128> log_buffer;
    Writer::Textual log_writer(log_buffer.get_access());
    log_writer << "Compression: unknown compression level "_view
               << Bits_8(level)
               << " requested, defaulting to Level::Default (2)";
    Diagnostics::Log::warning(log_writer);
  }
  case Level::Default:
    return deflate_fixed(source, default_search_depth);
  case Level::Best:
    return deflate_dynamic(source, best_search_depth);
  }
}

}  // namespace Perimortem::System::Compression
