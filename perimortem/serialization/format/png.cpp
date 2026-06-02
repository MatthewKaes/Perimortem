// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/format/png.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/system/compression/deflate.hpp"

#include "perimortem/graphics/image.hpp"

namespace Graphics = Perimortem::Graphics;

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;

// The color encoding used in a PNG file's IHDR chunk.
enum class ColorType : Bits_8 {
  Greyscale = 0,
  Rgb = 2,
  Indexed = 3,
  GreyscaleAlpha = 4,
  Rgba = 6,
};

// CRC-32 (Cyclic Redundancy Check): a 32-bit error-detection hash used by the
// PNG specification to verify each chunk has not been corrupted in transit.
// The IEEE 802.3 polynomial (0xEDB88320) is the standard choice for CRC-32.
constexpr Bits_32 ieee_crc32_polynomial = 0xEDB88320;

// CRC computation starts with all bits set; the result is XORed with this
// value again at the end to produce the final checksum.
constexpr Bits_32 crc32_initial_value = 0xFFFFFFFF;

static constexpr auto populate_crc_table() -> Static::Vector<Bits_32, 256> {
  Static::Vector<Bits_32, 256> table;
  for (Bits_32 table_index = 0; table_index < 256; table_index++) {
    Bits_32 crc_value = table_index;
    for (Count bit_index = 0; bit_index < 8; bit_index++) {
      crc_value = (crc_value & 1) ? (ieee_crc32_polynomial ^ (crc_value >> 1))
                                  : (crc_value >> 1);
    }
    table[table_index] = crc_value;
  }
  return table;
}

// The 256 CRC-32 look up table is prepopulated at compile time.
static constexpr Static::Vector<Bits_32, 256> crc_table = populate_crc_table();

static constexpr auto calculate_crc32(View::Bytes data) -> Bits_32 {
  Bits_32 running_crc = crc32_initial_value;
  for (Count byte_index = 0; byte_index < data.get_size(); byte_index++) {
    running_crc =
        crc_table[(running_crc ^ data[byte_index]) & 0xFF] ^ (running_crc >> 8);
  }
  return running_crc ^ crc32_initial_value;
}

// The 8-byte magic signature that begins every valid PNG file.
static constexpr Static::Bytes<8> png_signature = {0x89, 0x50, 0x4E, 0x47,
                                                   0x0D, 0x0A, 0x1A, 0x0A};

// A parsed PNG chunk. PNG files are made of sequential chunks, each with a
// type tag, a payload, and a CRC-32 checksum for integrity verification.
struct Chunk {
 public:
  Chunk() = default;
  Chunk(View::Bytes data, Static::Bytes<4> type, Bits_32 length)
      : data(data), type(type), length(length), valid(True) {}

  auto get_data() const -> View::Bytes { return data; }
  // 4-byte ASCII type tag, e.g. "IHDR", "IDAT", "IEND", "PLTE".
  auto get_type() const -> View::Bytes { return type.get_view(); }
  auto get_length() const -> Bits_32 { return length; }
  auto get_valid() const -> Bool { return valid; }

 private:
  View::Bytes data;
  Static::Bytes<4> type;
  Bits_32 length = 0;
  Bool valid = False;
};

static auto read_chunk(View::Bytes source, Count offset) -> Chunk {
  // Each chunk: 4 bytes length + 4 bytes type + data + 4 bytes CRC-32.
  constexpr Count chunk_overhead = 12;
  if (offset + chunk_overhead > source.get_size()) [[unlikely]] {
    Static::Bytes<96> log_buffer;
    Writer::Textual log_writer(log_buffer.get_access());
    log_writer << "Png: chunk at offset "_view << ULong(offset)
               << " truncated before header end"_view;
    Diagnostics::Log::error(log_writer);
    return Chunk();
  }

  Bits_32 length =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(source.get_data() + offset));

  if (offset + chunk_overhead + length > source.get_size()) [[unlikely]] {
    Static::Bytes<128> log_buffer;
    Writer::Textual log_writer(log_buffer.get_access());
    log_writer << "Png: chunk at offset "_view << ULong(offset)
               << " with length "_view << ULong(length)
               << " extends past end of stream"_view;
    Diagnostics::Log::error(log_writer);
    return Chunk();
  }

  Static::Bytes<4> type(source.slice(offset + 4, 4));
  View::Bytes data = source.slice(offset + 8, length);

  Bits_32 stored_crc =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(source.get_data() + offset + 8 + length));
  Bits_32 actual_crc = calculate_crc32(source.slice(offset + 4, 4 + length));
  if (stored_crc != actual_crc) [[unlikely]] {
    Static::Bytes<96> log_buffer;
    Writer::Textual log_writer(log_buffer.get_access());
    log_writer << "Png: CRC-32 mismatch for chunk '"_view << type.get_view()
               << "' at offset "_view << ULong(offset);
    Diagnostics::Log::error(log_writer);
    return Chunk();
  }

  return Chunk(data, type, length);
}

static auto number_of_color_channels(ColorType color_type) -> Count {
  switch (color_type) {
  case ColorType::Indexed:
  case ColorType::Greyscale:
    return 1;
  case ColorType::GreyscaleAlpha:
    return 2;
  case ColorType::Rgb:
    return 3;
  case ColorType::Rgba:
    return 4;
  default:
    return 0;
  }
}

// The Paeth predictor estimates the next sample by computing a  linear
// extrapolation p = left + up − upper_left, then returning whichever of the
// three values are closest to the current pixel.
static auto paeth_predictor(
    Bits_32 left_pixel,
    Bits_32 up_pixel,
    Bits_32 upper_left_pixel) -> Bits_8 {
  SignedBits_32 left = SignedBits_32(left_pixel);
  SignedBits_32 up = SignedBits_32(up_pixel);
  SignedBits_32 upper_left = SignedBits_32(upper_left_pixel);
  SignedBits_32 predictor = left + up - upper_left;
  SignedBits_32 left_dist =
      (predictor - left < 0) ? -(predictor - left) : (predictor - left);
  SignedBits_32 up_dist =
      (predictor - up < 0) ? -(predictor - up) : (predictor - up);
  SignedBits_32 corner_dist = (predictor - upper_left < 0)
                                  ? -(predictor - upper_left)
                                  : (predictor - upper_left);
  if (left_dist <= up_dist && left_dist <= corner_dist) {
    return Bits_8(left_pixel);
  }
  if (up_dist <= corner_dist) {
    return Bits_8(up_pixel);
  }
  return Bits_8(upper_left_pixel);
}

static auto reconstruct_filter(
    View::Bytes filtered_rows,
    Count width,
    Count height,
    Count bytes_per_pixel,
    Dynamic::Bytes& output) -> Bool {
  Count stride = width * bytes_per_pixel;
  // Each row: 1 filter byte followed by stride pixel bytes.
  constexpr Count filter_byte_size = 1;
  Count row_bytes = filter_byte_size + stride;

  if (filtered_rows.get_size() < row_bytes * height) [[unlikely]] {
    return False;
  }

  output.forgetful_resize(stride * height);
  auto acc = output.get_access();

  for (Count row = 0; row < height; row++) {
    Bits_8 filter_type = filtered_rows[row * row_bytes];
    Count row_source_offset = row * row_bytes + filter_byte_size;
    Count row_dest_offset = row * stride;

    for (Count col = 0; col < stride; col++) {
      Bits_8 raw_sample = filtered_rows[row_source_offset + col];

      // Note that for filters out of range bytes are actually Saturate8U to
      // zero rather than wrapping.
      Bits_32 left_pixel =
          (col >= bytes_per_pixel)
              ? Bits_32(acc[row_dest_offset + col - bytes_per_pixel])
              : 0;
      Bits_32 up_pixel =
          (row > 0) ? Bits_32(acc[row_dest_offset + col - stride]) : 0;
      Bits_32 upper_left_pixel =
          (row > 0 && col >= bytes_per_pixel)
              ? Bits_32(acc[row_dest_offset + col - stride - bytes_per_pixel])
              : 0;

      Bits_8 reconstructed = 0;
      switch (filter_type) {
      case 0:
        reconstructed = raw_sample;
        break;
      case 1:
        reconstructed = raw_sample + Bits_8(left_pixel);
        break;
      case 2:
        reconstructed = raw_sample + Bits_8(up_pixel);
        break;
      case 3:
        reconstructed = raw_sample + Bits_8((left_pixel + up_pixel) / 2);
        break;
      case 4:
        reconstructed = raw_sample +
                        paeth_predictor(left_pixel, up_pixel, upper_left_pixel);
        break;
      default:
        return False;
      }
      acc[row_dest_offset + col] = reconstructed;
    }
  }
  return True;
}

static auto convert_to_pixels(
    View::Bytes raw_pixels,
    Count width,
    Count height,
    ColorType color_type,
    View::Bytes palette,
    Dynamic::Vector<Graphics::Pixel>& output) -> Bool {
  Count source_channels = number_of_color_channels(color_type);
  if (source_channels == 0) [[unlikely]] {
    return False;
  }

  Count pixel_count = width * height;
  output.forgetful_resize(pixel_count);

  for (Count pixel_index = 0; pixel_index < pixel_count; pixel_index++) {
    Count source_offset = pixel_index * source_channels;
    switch (color_type) {
    case ColorType::Greyscale: {
      output[pixel_index] = Graphics::Pixel(raw_pixels[source_offset]);
    } break;
    case ColorType::Rgb: {
      output[pixel_index] = Graphics::Pixel(
          raw_pixels[source_offset + 0], raw_pixels[source_offset + 1],
          raw_pixels[source_offset + 2]);
    } break;
    case ColorType::Indexed: {
      Bits_8 palette_index = raw_pixels[source_offset];
      Count palette_offset = Count(palette_index) * 3;
      if (palette_offset + 3 > palette.get_size()) [[unlikely]] {
        return False;
      }
      output[pixel_index] = Graphics::Pixel(
          palette[palette_offset + 0], palette[palette_offset + 1],
          palette[palette_offset + 2]);
    } break;
    case ColorType::GreyscaleAlpha: {
      output[pixel_index] = Graphics::Pixel(
          raw_pixels[source_offset], raw_pixels[source_offset + 1]);
    } break;
    case ColorType::Rgba: {
      output[pixel_index] = Graphics::Pixel(
          raw_pixels[source_offset + 0], raw_pixels[source_offset + 1],
          raw_pixels[source_offset + 2], raw_pixels[source_offset + 3]);
    } break;
    default:
      return False;
    }
  }
  return True;
}

static auto write_chunk(
    Access::Bytes buffer,
    Count& write_position,
    View::Bytes type_tag,
    View::Bytes data) -> void {
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(buffer.get_data() + write_position),
      Bits_32(data.get_size()));
  write_position += 4;

  Data::copy(buffer.get_data() + write_position, type_tag.get_data(), Count(4));

  if (data.get_size() > 0) {
    Data::copy(
        buffer.get_data() + write_position + 4, data.get_data(),
        data.get_size());
  }

  Bits_32 chunk_crc = calculate_crc32(
      View::Bytes(buffer.get_data() + write_position, 4 + data.get_size()));
  write_position += 4 + data.get_size();
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(buffer.get_data() + write_position), chunk_crc);
  write_position += 4;
}

// Metadata from a PNG file's IHDR chunk, readable without decoding pixels.
struct ImageInfo {
 public:
  ImageInfo() = default;
  ImageInfo(
      Bits_32 width,
      Bits_32 height,
      Bits_8 bit_depth,
      ColorType color_type)
      : width(width),
        height(height),
        color_type(color_type),
        bit_depth(bit_depth),
        valid(True) {}

  auto get_width() const -> Bits_32 { return width; }
  auto get_height() const -> Bits_32 { return height; }
  auto get_bit_depth() const -> Bits_8 { return bit_depth; }
  auto get_color_type() const -> ColorType { return color_type; }
  auto get_valid() const -> Bool { return valid; }

 private:
  Bits_32 width = 0;
  Bits_32 height = 0;
  ColorType color_type = ColorType::Rgba;
  Bits_8 bit_depth = 0;
  Bool valid = False;
};

static auto read_info(const View::Bytes source) -> ImageInfo {
  constexpr Count png_signature_size = 8;
  // IHDR chunk: 4 length + 4 type + 13 data + 4 CRC-32 = 25 bytes.
  constexpr Count ihdr_chunk_size = 25;
  constexpr Count min_readable_size = png_signature_size + ihdr_chunk_size;
  constexpr Count ihdr_data_size = 13;
  constexpr Bits_8 required_zero_field = 0;
  constexpr Bits_8 supported_bit_depth = 8;

  if (source.get_size() < min_readable_size) [[unlikely]] {
    return ImageInfo();
  }

  if (source.slice(0, png_signature_size) != png_signature.get_view())
      [[unlikely]] {
    return ImageInfo();
  }

  // IHDR: the mandatory first chunk that defines image dimensions and format.
  Chunk ihdr = read_chunk(source, png_signature_size);
  if (!ihdr.get_valid() || ihdr.get_type() != "IHDR"_view ||
      ihdr.get_length() != ihdr_data_size) [[unlikely]] {
    return ImageInfo();
  }

  View::Bytes ihdr_data = ihdr.get_data();
  Bits_32 width =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(ihdr_data.get_data() + 0));
  Bits_32 height =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(ihdr_data.get_data() + 4));
  Bits_8 bit_depth = ihdr_data[8];
  ColorType color_type = ColorType(ihdr_data[9]);

  // Fields 10/11/12: compression method (must be 0 = deflate), filter method
  // (must be 0 = adaptive), and interlace method (must be 0 = no interlace).
  if (bit_depth != supported_bit_depth) [[unlikely]] {
    return ImageInfo();
  }
  if (number_of_color_channels(color_type) == 0) [[unlikely]] {
    return ImageInfo();
  }
  if (ihdr_data[10] != required_zero_field ||
      ihdr_data[11] != required_zero_field ||
      ihdr_data[12] != required_zero_field) [[unlikely]] {
    return ImageInfo();
  }

  return ImageInfo(width, height, bit_depth, color_type);
}

auto Format::Png::decode(const View::Bytes source) -> Graphics::Image {
  ImageInfo info = read_info(source);
  if (!info.get_valid()) [[unlikely]] {
    return Graphics::Image();
  }

  constexpr Count png_signature_size = 8;
  constexpr Count chunk_overhead = 12;

  Dynamic::Bytes compressed_data;
  View::Bytes palette;

  // Walk the chunk sequence collecting all IDAT payloads plus the optional
  // PLTE (palette) chunk needed for indexed-color images.
  Count chunk_offset = png_signature_size;
  while (chunk_offset < source.get_size()) {
    Chunk chunk = read_chunk(source, chunk_offset);
    if (!chunk.get_valid()) [[unlikely]] {
      return Graphics::Image();
    }
    if (chunk.get_type() == "IEND"_view) {
      break;
    }

    if (chunk.get_type() == "IDAT"_view) {
      // IDAT: image data compressed with DEFLATE (zlib wrapper, RFC 1951).
      compressed_data.concat(chunk.get_data());
    } else if (chunk.get_type() == "PLTE"_view) {
      // PLTE: palette for indexed-color images, 3 bytes (RGB) per entry.
      palette = chunk.get_data();
    }

    chunk_offset += chunk_overhead + chunk.get_length();
  }

  if (compressed_data.get_size() == 0) [[unlikely]] {
    return Graphics::Image();
  }

  // Inflate: decompress the zlib-wrapped DEFLATE stream from the IDAT chunks.
  Dynamic::Bytes filtered_rows =
      Compression::inflate(compressed_data.get_view());
  if (filtered_rows.get_size() == 0) [[unlikely]] {
    return Graphics::Image();
  }

  Count bytes_per_pixel = number_of_color_channels(info.get_color_type());
  Dynamic::Bytes raw_pixels;
  if (!reconstruct_filter(
          filtered_rows.get_view(), info.get_width(), info.get_height(),
          bytes_per_pixel, raw_pixels)) [[unlikely]] {
    return Graphics::Image();
  }

  Dynamic::Vector<Graphics::Pixel> pixels;
  if (!convert_to_pixels(
          raw_pixels.get_view(), info.get_width(), info.get_height(),
          info.get_color_type(), palette, pixels)) [[unlikely]] {
    return Graphics::Image();
  }

  return Graphics::Image(
      Data::take(pixels), info.get_width(), info.get_height());
}

auto Format::Png::encode(const Graphics::Image& image) -> Dynamic::Bytes {
  constexpr Count rgba_bytes_per_pixel = sizeof(Graphics::Pixel);
  constexpr Count filter_byte_size = 1;

  Bits_32 width = image.get_width();
  Bits_32 height = image.get_height();
  View::Vector<Graphics::Pixel> pixels = image.get_pixels();

  // Ignore empty images
  if (width == 0 || height == 0) [[unlikely]] {
    return Dynamic::Bytes();
  }
  if (pixels.get_size() != Count(width) * Count(height)) [[unlikely]] {
    return Dynamic::Bytes();
  }

  Count stride = Count(width) * rgba_bytes_per_pixel;
  Count filtered_size = Count(height) * (filter_byte_size + stride);
  Dynamic::Bytes filtered_rows(filtered_size);
  filtered_rows.forgetful_resize(filtered_size);
  auto filtered_acc = filtered_rows.get_access();

  // Encode each row with filter type 0 (None): pixels written verbatim.
  // Pixel layout in memory is {red, green, blue, alpha} matching PNG RGBA.
  auto raw_bytes = Data::cast<Bits_8>(pixels.get_data());
  for (Count row = 0; row < height; row++) {
    Count row_start = row * (filter_byte_size + stride);
    filtered_acc[row_start] = 0;
    Data::copy(
        filtered_acc.get_data() + row_start + filter_byte_size,
        raw_bytes + row * stride, stride);
  }

  // Deflate: compress the filtered pixel data using stored (level-0) DEFLATE.
  Dynamic::Bytes compressed = Compression::deflate(filtered_rows.get_view());
  if (compressed.get_size() == 0) [[unlikely]] {
    return Dynamic::Bytes();
  }

  // signature(8) + IHDR chunk(25) + IDAT chunk(12 + N) + IEND chunk(12).
  constexpr Count png_signature_size = 8;
  constexpr Count ihdr_chunk_size = 25;
  constexpr Count iend_chunk_size = 12;
  constexpr Count chunk_overhead = 12;
  Count output_size = png_signature_size + ihdr_chunk_size + chunk_overhead +
                      compressed.get_size() + iend_chunk_size;
  Dynamic::Bytes output(output_size);
  output.forgetful_resize(output_size);
  auto acc = output.get_access();

  Data::copy(acc.get_data(), png_signature.get_data(), png_signature_size);
  Count write_position = png_signature_size;

  // Build the 13-byte IHDR payload.
  constexpr Count ihdr_data_size = 13;
  constexpr Bits_8 bit_depth_8bpc = 8;
  constexpr Bits_8 no_compression = 0;
  constexpr Bits_8 no_filter = 0;
  constexpr Bits_8 no_interlace = 0;
  Static::Bytes<ihdr_data_size> ihdr_buffer;
  auto ihdr_acc = ihdr_buffer.get_access();
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(ihdr_acc.get_data() + 0), width);
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(ihdr_acc.get_data() + 4), height);
  ihdr_acc[8] = bit_depth_8bpc;
  ihdr_acc[9] = Bits_8(ColorType::Rgba);
  ihdr_acc[10] = no_compression;
  ihdr_acc[11] = no_filter;
  ihdr_acc[12] = no_interlace;

  write_chunk(acc, write_position, "IHDR"_view, ihdr_buffer.get_view());
  write_chunk(acc, write_position, "IDAT"_view, compressed.get_view());
  write_chunk(acc, write_position, "IEND"_view, View::Bytes());

  output.resize(write_position);
  return output;
}
