// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/format/png.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
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

// Per-row prediction applied before DEFLATE. Each value matches the filter
// byte written at the start of the filtered row per the PNG specification.
enum class FilterType : Bits_8 {
  None = 0,
  Sub = 1,
  Up = 2,
  Average = 3,
  Paeth = 4,
};

// ImageInfo follows the Binary layout of an IHDR chunk.
struct ImageInfo {
 public:
  constexpr ImageInfo() = default;
  constexpr ImageInfo(const Graphics::Image& image) {
    Data::write<Data::ByteOrder::Big>(&width, image.get_width());
    Data::write<Data::ByteOrder::Big>(&height, image.get_height());
    bit_depth = 8;
    color_type = ColorType::Rgba;
  }

  // C++ will add padding to the end of the struct so store the actual size.
  static constexpr Count size = 13;

  constexpr auto get_width() const -> Bits_32 {
    return Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
        width);
  }
  constexpr auto get_height() const -> Bits_32 {
    return Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
        height);
  }
  constexpr auto get_bit_depth() const -> Bits_8 { return bit_depth; }
  constexpr auto get_color_type() const -> ColorType { return color_type; }
  constexpr auto is_valid() const -> Bool {
    return compression_method == 0 && filter_method == FilterType::None &&
           interlace_method == 0;
  }

 private:
  Bits_32 width = 0;
  Bits_32 height = 0;
  Bits_8 bit_depth = 0;
  ColorType color_type = ColorType::Rgba;
  Bits_8 compression_method = 0;
  FilterType filter_method = FilterType::None;
  Bits_8 interlace_method = 0;
};

// A parsed PNG chunk. PNG files are made of sequential chunks, each with a
// type tag, a payload, and a CRC-32 checksum for integrity verification.
class Chunk {
 public:
  constexpr Chunk() = default;
  constexpr Chunk(View::Bytes data, Static::Bytes<4> type, Bits_32 length)
      : data(data), type(type), length(length), valid(True) {}

  constexpr auto get_data() const -> View::Bytes { return data; }
  constexpr auto get_type() const -> View::Bytes { return type.get_view(); }
  constexpr auto get_length() const -> Bits_32 { return length; }
  constexpr auto get_valid() const -> Bool { return valid; }

 private:
  View::Bytes data;
  Static::Bytes<4> type;
  Bits_32 length = 0;
  Bool valid = False;
};

// The 8-byte magic signature that begins every valid PNG file.
constexpr Static::Bytes<8> png_signature = {0x89, 0x50, 0x4E, 0x47,
                                            0x0D, 0x0A, 0x1A, 0x0A};

// Each chunk: 4 bytes length + 4 bytes type + data + 4 bytes CRC-32.
constexpr Count chunk_metadata_size = 12;

constexpr auto number_of_color_channels(ColorType color_type) -> Count {
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

constexpr auto populate_crc_table() -> Static::Vector<Bits_32, 256> {
  // CRC-32 (Cyclic Redundancy Check): a 32-bit error-detection hash used by the
  // PNG specification to verify each chunk has not been corrupted in transit.
  // The IEEE 802.3 polynomial (0xEDB88320) is the standard choice for CRC-32.
  constexpr Bits_32 ieee_crc32_polynomial = 0xEDB88320;

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

constexpr auto calculate_crc32(View::Bytes data) -> Bits_32 {
  // The 256 CRC-32 look up table is prepopulated at compile time.
  constexpr Static::Vector<Bits_32, 256> crc_table = populate_crc_table();

  // CRC computation starts with all bit set.
  Bits_32 running_crc = 0xFFFFFFFF;
  for (Count byte_index = 0; byte_index < data.get_size(); byte_index++) {
    running_crc =
        crc_table[(running_crc ^ data[byte_index]) & 0xFF] ^ (running_crc >> 8);
  }

  // XOR the bits to produce the final CRC value.
  return running_crc ^ 0xFFFFFFFF;
}

constexpr auto read_chunk(View::Bytes source, Count offset) -> Chunk {
  if (offset + chunk_metadata_size > source.get_size()) [[unlikely]] {
    Static::Bytes<96> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: Chunk at offset "_view << Signed_64(offset)
               << " truncated before header end"_view;
    Diagnostics::Log::error(log_writer);
    return Chunk();
  }

  Bits_32 length =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(source.get_data() + offset));

  if (offset + chunk_metadata_size + length > source.get_size()) [[unlikely]] {
    Static::Bytes<128> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: Chunk at offset "_view << Signed_64(offset)
               << " with length "_view << Signed_64(length)
               << " extends past end of stream"_view;
    Diagnostics::Log::error(log_writer);
    return Chunk();
  }

  Static::Bytes<4> type(source.slice(offset + 4, 4));
  View::Bytes data = source.slice(offset + 8, length);

  // Caculating the CRC from the buffer is slow and is unnecessary for files
  // from a trusted source. The check is still used in debug builds, but in
  // release builds removing the CRC check improves throughput by as much as
  // 20% for large images.
#if PERI_DEBUG
  Bits_32 stored_crc =
      Data::ensure_endian<Data::ByteOrder::Big, Data::ByteOrder::Native>(
          *Data::cast<Bits_32>(source.get_data() + offset + 8 + length));
  Bits_32 actual_crc = calculate_crc32(source.slice(offset + 4, 4 + length));
  if (stored_crc != actual_crc) [[unlikely]] {
    Static::Bytes<96> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: CRC-32 mismatch for chunk '"_view << type.get_view()
               << "' at offset "_view << Signed_64(offset);
    Diagnostics::Log::error(log_writer);
    return Chunk();
  }
#endif

  return Chunk(data, type, length);
}

constexpr auto write_chunk(
    Access::Bytes buffer,
    Count& write_position,
    View::Bytes type_tag,
    View::Bytes data) -> void {
  auto output = buffer.get_data();

  // Write the size in Big endian format
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(output + write_position), Bits_32(data.get_size()));
  write_position += sizeof(Bits_32);

  // Copy the tag into the buffer.
  Data::copy(output + write_position, type_tag.get_data(), type_tag.get_size());
  write_position += type_tag.get_size();

  // If data isn't empty for the chunk then write that next.
  Data::copy(output + write_position, data.get_data(), data.get_size());
  write_position += data.get_size();

  // Finally write out the crc32 value (also in big endian) which includes the
  // tag and any other chunk data.
  Count chunk_length = type_tag.get_size() + data.get_size();
  Bits_32 chunk_crc = calculate_crc32(
      View::Bytes(output + write_position - chunk_length, chunk_length));
  Data::write<Data::ByteOrder::Big>(
      Data::cast<Bits_32>(output + write_position), chunk_crc);
  write_position += 4;
}

// The Paeth predictor estimates the next sample by computing a linear
// extrapolation using the left, upper, and upper left pixels and returning the
// value closest to the current pixel.
constexpr auto paeth_predictor(Static::Vector<Bits_8, 3> pixels) -> Bits_8 {
  Signed_16 left = Signed_16(pixels[0]);
  Signed_16 up = Signed_16(pixels[1]);
  Signed_16 upper_left = Signed_16(pixels[2]);
  Signed_16 predictor = left + up - upper_left;

  Signed_16 score_left = Math::absolute(predictor - left);
  Signed_16 score_up = Math::absolute(predictor - up);
  Signed_16 score_upper_left = Math::absolute(predictor - upper_left);

  if (score_left <= score_up && score_left <= score_upper_left) {
    return pixels[0];
  }

  if (score_up <= score_upper_left) {
    return pixels[1];
  }

  return pixels[2];
}

// Smaller residuals _generally_ compress better under DEFLATE.
// We treat each byte as a signed value and sum the magnitudes as a cheap proxy
// estimate for compressibility.
template <Bool first_row>
auto score_row(View::Bytes current, View::Bytes prev) -> FilterType {
  constexpr auto signed_abs = Math::absolute<Signed_8>;
  constexpr auto filter_count = first_row ? 2 : 5;
  Static::Vector<Bits_64, filter_count> scores;

  auto current_data = current.get_data();
  auto prev_data = prev.get_data();

  // Process the first column of data.
  for (Count i = 0; i < Graphics::Image::get_channel_count(); i++) {
    scores[Bits_8(FilterType::None)] += signed_abs(Signed_8(current_data[i]));
    scores[Bits_8(FilterType::Sub)] += signed_abs(Signed_8(current_data[i]));

    if constexpr (!first_row) {
      Bits_8 up = prev_data[i];
      scores[Bits_8(FilterType::Up)] += signed_abs(current_data[i] - up);
      scores[Bits_8(FilterType::Average)] +=
          signed_abs(current_data[i] - up / 2);
      scores[Bits_8(FilterType::Paeth)] += signed_abs(current_data[i] - up);
    }
  }

  for (Count i = Graphics::Image::get_channel_count(); i < current.get_size();
       i++) {
    Bits_8 left = current_data[i - Graphics::Image::get_channel_count()];
    scores[Bits_8(FilterType::None)] += signed_abs(Signed_8(current_data[i]));
    scores[Bits_8(FilterType::Sub)] +=
        Math::absolute<Signed_8>(current_data[i] - left);
    if constexpr (!first_row) {
      Bits_8 up = prev_data[i];
      Bits_8 upper_left = prev_data[i - Graphics::Image::get_channel_count()];
      scores[Bits_8(FilterType::Up)] += signed_abs(current_data[i] - up);
      scores[Bits_8(FilterType::Average)] +=
          signed_abs(current_data[i] - (Bits_16(left) + Bits_16(up)) / 2);
      scores[Bits_8(FilterType::Paeth)] +=
          signed_abs(current_data[i] - paeth_predictor({left, up, upper_left}));
    }
  }

  return FilterType(Algorithm::min_element<Bits_64>(scores));
}

// Applies a PNG forward filter to one row of pixels, writing residuals into
// out. prev is the previous unfiltered row and is empty for the first row.
//
// If prev isn't a valid row then the function will corrupt memory so it's an
// important precondition to not mess up.
constexpr auto apply_row_filter(
    FilterType filter_type,
    Access::Bytes out,
    View::Bytes current,
    View::Bytes prev) -> void {
  Count size = current.get_size();

  auto out_data = out.get_data();
  auto current_data = current.get_data();
  auto prev_data = prev.get_data();

#if PERI_DEBUG
  // All of the buffers should be appropriately sized so just work on the raw
  // data directly since this is hot path code.
  if (out.get_size() != size || prev.get_size() != size) {
    Diagnostics::Log::fatal(
        "PNG: apply_filter_row was called with one or more invalid rows."_view);
    return;
  }
#endif

  // Special case the first pixel in the row
  switch (filter_type) {
  case FilterType::Sub:
    Data::copy(out_data, current_data, Graphics::Image::get_channel_count());
    break;
  case FilterType::Up:
    for (Count i = 0; i < Graphics::Image::get_channel_count(); i++) {
      out_data[i] = Bits_8(current_data[i] - prev_data[i]);
    }
    break;
  case FilterType::Average:
    for (Count i = 0; i < Graphics::Image::get_channel_count(); i++) {
      out_data[i] = current_data[i] - Bits_8(prev_data[i]) / 2;
    }
    break;
  case FilterType::Paeth:
    for (Count i = 0; i < Graphics::Image::get_channel_count(); i++) {
      out_data[i] =
          Bits_8(current_data[i] - paeth_predictor({0, prev_data[i], 0}));
    }
    break;

    // Treat default as None for now, but we should most likely log an error.
  case FilterType::None:
  default:
    Data::copy(out_data, current_data, size);
    return;
  }

  switch (filter_type) {
  case FilterType::Sub:
    for (Count i = Graphics::Image::get_channel_count(); i < size; i++) {
      Bits_8 left = current_data[i - Graphics::Image::get_channel_count()];
      out_data[i] = Bits_8(current_data[i] - left);
    }
    break;
  case FilterType::Up:
    for (Count i = Graphics::Image::get_channel_count(); i < size; i++) {
      out_data[i] = Bits_8(current_data[i] - prev_data[i]);
    }
    break;
  case FilterType::Average:
    for (Count i = Graphics::Image::get_channel_count(); i < size; i++) {
      Bits_8 left = current_data[i - Graphics::Image::get_channel_count()];
      Bits_8 up = prev_data[i];
      out_data[i] =
          Bits_8(current_data[i] - Bits_8((Bits_32(left) + Bits_32(up)) / 2));
    }
    break;
  case FilterType::Paeth:
    for (Count i = Graphics::Image::get_channel_count(); i < size; i++) {
      Bits_8 left = current_data[i - Graphics::Image::get_channel_count()];
      Bits_8 up = prev_data[i];
      Bits_8 upper_left = prev_data[i - Graphics::Image::get_channel_count()];
      out_data[i] =
          Bits_8(current_data[i] - paeth_predictor({left, up, upper_left}));
    }
    break;

    // Treat default as None for now, but we should most likely log an error.
  case FilterType::None:
  default:
    Data::copy(out_data, current_data, size);
  }
}

// Applies adaptive PNG filtering to the source pixels, scoring all five filter
// types per row and selecting the lowest-residual option.
constexpr auto apply_adaptive_filtering(const Graphics::Image& image)
    -> Dynamic::Bytes {
  const auto row_stride =
      Count(image.get_width()) * Graphics::Pixel::get_byte_count();
  const auto output_size = Count(image.get_height()) * (1 + row_stride);
  const auto raw_bytes = Data::cast<Bits_8>(image.get_pixels().get_data());

  // Create the full output buffer and resize it to the full size.
  Dynamic::Bytes output(output_size);
  output.forgetful_resize(output_size);

  // For the first row can only use one of two filters so simply score those.
  // We use the current row as the "prev" row since it's not used.
  View::Bytes current_row(raw_bytes, row_stride);
  FilterType best_filter = score_row<True>(current_row, current_row);

  // Apply the filtering to the first row and set the filter used.
  apply_row_filter(
      best_filter, output.get_access().slice(1, row_stride),
      View::Bytes(raw_bytes, row_stride), View::Bytes(raw_bytes, row_stride));
  output.get_access()[0] = Bits_8(best_filter);

  // The rest of the rows after the first are scored against all filters.
  for (Count row = 1; row < image.get_height(); row++) {
    // Update the previous and current rows.
    View::Bytes prev_row = current_row;
    current_row = View::Bytes(raw_bytes + row * row_stride, row_stride);

    // Apply the lowest scoring filter to the row so we only write to the output
    // buffer once. This saves a ton of memory traffic as we minimize the number
    // of actual writes.
    const auto best_filter = score_row<False>(current_row, prev_row);
    Access::Bytes out_row =
        output.get_access().slice(row * (1 + row_stride) + 1, row_stride);
    apply_row_filter(best_filter, out_row, current_row, prev_row);
    output.get_access()[row * (1 + row_stride)] = Bits_8(best_filter);
  }

  return output;
}

// Reconstructs a single filtered row into dst with a minor optimization that
// statically compiles away usage of the `up` pointer when processing the first
// row, while guaranteeing that `up` is always valid for the following rows.
//
// The code takes raw Bits_8* pointers and is hand optimized since it's the main
// hot loop for the load path which is the main use case for production builds.
template <Bool first_row>
auto reconstruct_row(
    FilterType filter_type,
    const Bits_8* src,
    Bits_8* dst,
    const Bits_8* up,
    Count stride,
    Count bytes_per_pixel) -> Bool {
  switch (filter_type) {
  case FilterType::None:
    Data::copy(dst, src, stride);
    break;

  case FilterType::Sub:
    Data::copy(dst, src, bytes_per_pixel);

    for (Count i = bytes_per_pixel; i < stride; i++) {
      dst[i] = src[i] + dst[i - bytes_per_pixel];
    }
    break;

  case FilterType::Up:
    // If we are processing the first row then we can simply copy the data as
    // the `up` values are saturated to 0.
    if constexpr (first_row) {
      Data::copy(dst, src, stride);
    } else {
      for (Count i = 0; i < stride; i++) {
        dst[i] = src[i] + up[i];
      }
    }
    break;

  case FilterType::Average:
    if constexpr (first_row) {
      Data::copy(dst, src, bytes_per_pixel);

      for (Count i = bytes_per_pixel; i < stride; i++) {
        dst[i] = src[i] + Bits_8(Bits_32(dst[i - bytes_per_pixel]) / 2);
      }
    } else {
      // First column
      for (Count i = 0; i < bytes_per_pixel; i++) {
        dst[i] = src[i] + Bits_8(Bits_32(up[i]) / 2);
      }

      // Rest of the row
      for (Count i = bytes_per_pixel; i < stride; i++) {
        dst[i] =
            src[i] +
            Bits_8((Bits_32(dst[i - bytes_per_pixel]) + Bits_32(up[i])) / 2);
      }
    }
    break;

  case FilterType::Paeth:
    // For the first row this reduces to Sub.
    if constexpr (first_row) {
      Data::copy(dst, src, bytes_per_pixel);
      for (Count i = bytes_per_pixel; i < stride; i++) {
        dst[i] = src[i] + dst[i - bytes_per_pixel];
      }
    } else {
      // First column
      for (Count i = 0; i < bytes_per_pixel; i++) {
        dst[i] = src[i] + up[i];
      }

      // Rest of the row
      for (Count i = bytes_per_pixel; i < stride; i++) {
        dst[i] = src[i] + paeth_predictor(
                              {dst[i - bytes_per_pixel], up[i],
                               up[i - bytes_per_pixel]});
      }
    }
    break;

    // For all unknown values error out.
  default: {
    Static::Bytes<64> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: Unknown filter type "_view << Bits_32(filter_type);
    Diagnostics::Log::error(log_writer);
    return False;
  }
  }

  return True;
}

constexpr auto reconstruct_filter(
    View::Bytes filtered_rows,
    Count width,
    Count height,
    Count bytes_per_pixel,
    Access::Bytes output) -> Bool {
  const Count stride = width * bytes_per_pixel;
  const Count row_bytes = 1 + stride;
  const auto filter_data = filtered_rows.get_data();
  auto data = output.get_data();

  if (filtered_rows.get_size() < row_bytes * height) [[unlikely]] {
    Static::Bytes<128> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: Decompressed size "_view << filtered_rows.get_size()
               << " is smaller than required "_view << (row_bytes * height)
               << " bytes for this image"_view;
    Diagnostics::Log::error(log_writer);
    return False;
  }

  // First row compiles away all null checks.
  FilterType filter_type = FilterType(filter_data[0]);
  if (!reconstruct_row<True>(
          filter_type, filter_data + 1, data, nullptr, stride, bytes_per_pixel))
      [[unlikely]] {
    return False;
  }

  // Now that `up` is always valid no runtime null checks needed.
  for (Count row = 1; row < height; row++) {
    FilterType filter_type = FilterType(filter_data[row * row_bytes]);
    const Bits_8* src = filter_data + row * row_bytes + 1;
    Bits_8* dst = data + row * stride;
    const Bits_8* up = dst - stride;
    if (!reconstruct_row<False>(
            filter_type, src, dst, up, stride, bytes_per_pixel)) [[unlikely]] {
      return False;
    }
  }

  return True;
}

constexpr auto convert_to_pixels(
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
  auto data = raw_pixels.get_data();

  for (Count pixel_index = 0; pixel_index < pixel_count; pixel_index++) {
    Count source_offset = pixel_index * source_channels;
    switch (color_type) {
    case ColorType::Greyscale:
      output[pixel_index] = Graphics::Pixel(data[source_offset]);
      break;
    case ColorType::GreyscaleAlpha:
      output[pixel_index] =
          Graphics::Pixel(data[source_offset], data[source_offset + 1]);
      break;
    case ColorType::Rgb:
      output[pixel_index] = Graphics::Pixel(
          data[source_offset + 0], data[source_offset + 1],
          data[source_offset + 2]);
      break;
    case ColorType::Indexed: {
      Bits_8 palette_index = data[source_offset];
      Count palette_offset = Count(palette_index) * 3;

      // If the color is outside the palette size then error out.
      if (palette_offset + 3 > palette.get_size()) [[unlikely]] {
        Static::Bytes<128> error_buffer;
        Writer::Textual log_writer(error_buffer.get_access());
        log_writer << "Png: Palette index "_view << Bits_32(palette_index)
                   << " at pixel "_view << Signed_64(pixel_index)
                   << " exceeds palette size "_view
                   << Signed_64(palette.get_size() / 3);
        Diagnostics::Log::error(log_writer);
        return False;
      }

      output[pixel_index] = Graphics::Pixel(
          palette[palette_offset + 0], palette[palette_offset + 1],
          palette[palette_offset + 2]);
      break;
    }

      // RGBA should be fast pathed by the decoder to avoid a copy.
    case ColorType::Rgba:
    default:
      return False;
    }
  }
  return True;
}

constexpr auto read_header(const View::Bytes source) -> ImageInfo {
  // Check if the header is smaller than the png signiture + header chunk.
  constexpr Count required_size =
      png_signature.get_size() + chunk_metadata_size + ImageInfo::size;
  if (source.get_size() < required_size) [[unlikely]] {
    return ImageInfo();
  }

  // Validate the PNG signiture
  if (source.slice(0, png_signature.get_size()) != png_signature.get_view())
      [[unlikely]] {
    Diagnostics::Log::error("Png: Invalid file signature"_view);
    return ImageInfo();
  }

  // Check that the first tag is IHDR
  constexpr auto header_tag = "IHDR"_view;
  auto chunk_tag = source.slice(
      png_signature.get_size() + sizeof(Bits_32), header_tag.get_size());
  if (chunk_tag != "IHDR"_view) [[unlikely]] {
    Static::Bytes<96> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: First chunk must be \"IHDR\". header="_view
               << chunk_tag;
    Diagnostics::Log::error(log_writer);

    return ImageInfo();
  }

  // Read the image info.
  auto image_info = *Data::cast<const ImageInfo>(
      source
          .slice(
              png_signature.get_size() + sizeof(Bits_32) +
                  header_tag.get_size(),
              ImageInfo::size)
          .get_data());

  // Currently only support 8 bit color depth
  if (image_info.get_bit_depth() != Graphics::Image::get_color_depth())
      [[unlikely]] {
    Static::Bytes<96> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: Unsupported bit depth "_view
               << Bits_32(image_info.get_bit_depth())
               << " (only 8 bit is supported)"_view;
    Diagnostics::Log::error(log_writer);

    return ImageInfo();
  }

  const ColorType color_type = image_info.get_color_type();
  if (number_of_color_channels(color_type) == 0) [[unlikely]] {
    Static::Bytes<96> error_buffer;
    Writer::Textual log_writer(error_buffer.get_access());
    log_writer << "Png: Unsupported color type "_view << Bits_32(color_type);
    Diagnostics::Log::error(log_writer);
    return ImageInfo();
  }

  // Check the rest of the data is set correctly.
  if (!image_info.is_valid()) [[unlikely]] {
    Diagnostics::Log::error(
        "Png: Non-standard compression, filter, or interlace method in IHDR"_view);
    return ImageInfo();
  }

  return image_info;
}

constexpr auto process_data(
    const ImageInfo info,
    View::Bytes source,
    View::Bytes palette) -> Graphics::Image {
  // The exact decompressed size can be derived from the image info so we can
  // use that to preallocate the buffer.
  Count bytes_per_pixel = number_of_color_channels(info.get_color_type());
  const Count decompressed_capacity =
      info.get_width() * info.get_height() * bytes_per_pixel +
      /* Extra filter byte per row */ info.get_height();
  Dynamic::Bytes filtered_rows =
      Compression::Deflate::inflate(source, decompressed_capacity);
  if (filtered_rows.get_size() == 0) [[unlikely]] {
    return Graphics::Image();
  }

  // If the target format is our desired format then we can reconstruct the data
  // in place which saves a copy.
  if (info.get_color_type() == ColorType::Rgba) {
    Dynamic::Vector<Graphics::Pixel> pixels;
    pixels.forgetful_resize(info.get_width() * info.get_height());
    if (!reconstruct_filter(
            filtered_rows.get_view(), info.get_width(), info.get_height(),
            bytes_per_pixel, pixels.get_access().get_bytes())) [[unlikely]] {
      Diagnostics::Log::error(
          "Png: Filter reconstruction failed — decompressed data may be truncated"_view);
      return Graphics::Image();
    }

    return Graphics::Image(
        Data::take(pixels), info.get_width(), info.get_height());
  }

  // If we can't construct in place then we have to use a temp buffer to store
  // the intermediate data.
  Dynamic::Bytes raw_pixels;
  raw_pixels.forgetful_resize(
      info.get_width() * info.get_height() * bytes_per_pixel);
  if (!reconstruct_filter(
          filtered_rows.get_view(), info.get_width(), info.get_height(),
          bytes_per_pixel, raw_pixels)) [[unlikely]] {
    Diagnostics::Log::error(
        "Png: Filter reconstruction failed — decompressed data may be truncated"_view);
    return Graphics::Image();
  }

  // Convert the arbitrary color format into Pixel's RGBA format.
  Dynamic::Vector<Graphics::Pixel> pixels;
  if (!convert_to_pixels(
          raw_pixels.get_view(), info.get_width(), info.get_height(),
          info.get_color_type(), palette, pixels)) [[unlikely]] {
    Diagnostics::Log::error("Png: Pixel conversion failed"_view);
    return Graphics::Image();
  }

  return Graphics::Image(
      Data::take(pixels), info.get_width(), info.get_height());
}

auto Format::Png::decode(const View::Bytes source) -> Graphics::Image {
  // If we can't load the image or if the image is empty then return empty.
  ImageInfo info = read_header(source);
  if (info.get_width() == 0 || info.get_height() == 0) [[unlikely]] {
    return Graphics::Image();
  }

  // For PNGs with a single IDAT chunk (the common case) we hold a view to avoid
  // copying the entire compressed stream, but if any additional IDAT chunks are
  // found we switch to merging them into a dynamic buffer.
  View::Bytes binary_data;
  Dynamic::Bytes merged_data;
  View::Bytes palette;

  // Walk the chunk sequence collecting all IDAT payloads plus the optional
  // PLTE (palette) chunk needed for indexed-color images.
  Count chunk_offset = png_signature.get_size();
  while (chunk_offset < source.get_size()) {
    Chunk chunk = read_chunk(source, chunk_offset);
    if (!chunk.get_valid()) [[unlikely]] {
      return Graphics::Image();
    }

    if (chunk.get_type() == "IEND"_view) {
      break;
    }

    if (chunk.get_type() == "IDAT"_view) {
      // If we don't have any binary data we can just use a view.
      // Otherwise start building up the merged view.
      if (binary_data.empty()) {
        binary_data = chunk.get_data();
      } else {
        // Need to to merge in the current binary data from the last read.
        if (merged_data.is_empty()) {
          merged_data.proxy(binary_data);
        }

        // Add the chunk to the merged binary and update the binary view.
        merged_data.concat(chunk.get_data());
        binary_data = merged_data.get_view();
      }
    } else if (chunk.get_type() == "PLTE"_view) {
      palette = chunk.get_data();
    }

    chunk_offset += chunk_metadata_size + chunk.get_length();
  }

  if (binary_data.empty()) [[unlikely]] {
    Diagnostics::Log::error("Png: No IDAT chunk found or chunk was empty"_view);
    return Graphics::Image();
  }

  return process_data(info, binary_data, palette);
}

auto Format::Png::encode(const Graphics::Image& image) -> Dynamic::Bytes {
  View::Vector<Graphics::Pixel> pixels = image.get_pixels();

  // Ignore empty images
  if (pixels.empty()) [[unlikely]] {
    return Dynamic::Bytes();
  }

  // Filter the rows using adaptive PNG filtering then deflate the output.
  Dynamic::Bytes filtered_rows = apply_adaptive_filtering(image);

  // Compress using the default compression level for balanced speed vs size.
  Dynamic::Bytes compressed =
      Compression::Deflate::deflate(filtered_rows.get_view());
  if (compressed.get_size() == 0) [[unlikely]] {
    return Dynamic::Bytes();
  }

  // Output is just 3 chunks (header, end, data) and the PNG signiture.
  const auto output_size = png_signature.get_size() + ImageInfo::size +
                           (chunk_metadata_size * 3) + compressed.get_size();
  Dynamic::Bytes output(output_size);
  output.forgetful_resize(output_size);
  auto data = output.get_access();

  // Write the header & data chunk along with the IEND chunk.
  ImageInfo header(image);
  Data::copy(
      data.get_data(), png_signature.get_data(), png_signature.get_size());
  Count write_position = png_signature.get_size();
  write_chunk(
      data, write_position, "IHDR"_view,
      View::Bytes(Data::cast<Bits_8>(&header), ImageInfo::size));
  write_chunk(data, write_position, "IDAT"_view, compressed.get_view());
  write_chunk(data, write_position, "IEND"_view, View::Bytes());

  output.resize(write_position);
  return output;
}
