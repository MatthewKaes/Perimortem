// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/storage/serialization/textual/stream.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Serialization;

auto Textual::Stream::set_pointer(Count location) -> void {
  ptr_location = location;
}

auto Textual::Stream::operator<<(const Bits_8 bin) -> Stream& {
  return *this;
}

auto Textual::Stream::operator<<(const Bits_16 bin) -> Stream& {
  return *this;
}

auto Textual::Stream::operator<<(const Bits_32 bin) -> Stream& {
  return *this;
}

auto Textual::Stream::operator<<(const Bits_64 bin) -> Stream& {
  return *this;
}

auto Textual::Stream::operator<<(const SignedBits_8 bin) -> Stream& {
  return *this;
}

auto Textual::Stream::operator<<(const SignedBits_16 bin) -> Stream& {
  write_block<Textual::Stream::DataType::SignedBits_16>(data, ptr_location, bin);
  return *this;
}

auto Textual::Stream::operator<<(const SignedBits_32 bin) -> Stream& {
  write_block<Textual::Stream::DataType::SignedBits_32>(data, ptr_location, bin);
  return *this;
}

auto Textual::Stream::operator<<(const SignedBits_64 bin) -> Stream& {
  write_block<Textual::Stream::DataType::SignedBits_64>(data, ptr_location, bin);
  return *this;
}

auto Textual::Stream::operator<<(const Real_32 bin) -> Stream& {
  write_block<Textual::Stream::DataType::Real_32>(data, ptr_location, bin);
  return *this;
}

auto Textual::Stream::operator<<(const Real_64 bin) -> Stream& {
  write_block<Textual::Stream::DataType::Real_64>(data, ptr_location, bin);
  return *this;
}

auto Textual::Stream::operator<<(const View::Bytes bin) -> Stream& {
  return *this;
}

// //======================================================================
// // Optimized operations
// //======================================================================

// // Writes bytes directly into the buffer.
// template <typename storage_type, std::endian source_order =
// std::endian::little> auto read(Count location) const -> storage_type {
//   static_assert(
//       sizeof(storage_type) <= 8,
//       "Reading blocks larger than 8 bytes is typically an anti-pattern, "
//       "use buffers instead or smaller blocks if you care about endianness.");

//   if (location + sizeof(storage_type) > size)
//     return storage_type();

//   // When reading bytes we may not be aligned so we'll need to copy the
//   bytes. storage_type data; std::memcpy(&data, source_block + location,
//   sizeof(storage_type));

//   if constexpr (std::endian::native != std::endian::little) {
//     data = std::byteswap(data);
//   }

//   return data;
// }

// // Read with inplace incrementing counter.
// template <typename storage_type, std::endian source_order =
// std::endian::little> auto incremental_read(Count& location) const ->
// storage_type {
//   storage_type data = read<storage_type>(location);
//   location += sizeof(storage_type);
//   return data;
// }

// auto Bytes::append(Count number) -> void {
//   // Largest size required to represent any 64bit number.
//   constexpr auto number_range = 22;
//   char working_buffer[number_range] = {};
//   std::to_chars_result result =
//       std::to_chars(working_buffer, working_buffer + number_range, number);

//   if (result.ec != std::errc()) {
//     append(View::Bytes(reinterpret_cast<Byte*>(working_buffer),
//                        result.ptr - working_buffer));
//   } else {
//     append(Byte('0'));
//   }
// }

// auto Bytes::append(Long number) -> void {
//   // Largest size required to represent any 64bit number.
//   constexpr auto number_range = 22;
//   char working_buffer[number_range] = {};
//   std::to_chars_result result =
//       std::to_chars(working_buffer, working_buffer + number_range, number);

//   if (result.ec != std::errc()) {
//     append(View::Bytes(reinterpret_cast<Byte*>(working_buffer),
//                        result.ptr - working_buffer));
//   } else {
//     append(Byte('0'));
//   }
// }

// auto Bytes::append(Real_64 number) -> void {
//   // Largest size required to represent any 64bit number.
//   constexpr auto number_range = 28;
//   char working_buffer[number_range] = {};
//   std::to_chars_result result =
//       std::to_chars(working_buffer, working_buffer + number_range, number);

//   if (result.ec != std::errc()) {
//     append(View::Bytes(reinterpret_cast<Byte*>(working_buffer),
//                        result.ptr - working_buffer));
//   } else {
//     append(Byte('0'));
//   }
// }

// auto Bytes::serialize(Bits_8 data, Count location) {
//   if (size + sizeof(storage_type) > capacity) {
//     grow(sizeof(storage_type));
//   }

//   // When writing bytes we may not be aligned so we'll need to copy the
//   bytes. std::memcpy(rented_block + size, &data, sizeof(storage_type)); size
//   += sizeof(storage_type);
// }

// auto Bytes::serialize(Bits_16 data, Count location) {
//   if constexpr (std::endian::native != std::endian::little) {
//     data = std::byteswap(data);
//   }

//   if (size + sizeof(storage_type) > capacity) {
//     grow(sizeof(storage_type));
//   }

//   // When writing bytes we may not be aligned so we'll need to copy the
//   bytes. std::memcpy(rented_block + size, &data, sizeof(storage_type)); size
//   += sizeof(storage_type);
// }
// auto Bytes::serialize(Bits_32 data, Count location) {}
// auto Bytes::serialize(Bits_64 data, Count location) {}

// auto Managed::Bytes::append(View::Bytes view) -> void {
//   ensure_capacity(size + view.get_size());

//   std::memcpy(rented_block + size, view.get_data(), view.get_size());
//   size += view.get_size();
// }

// auto Managed::Bytes::append(Count number) -> void {
//   // Largest size required to represent any 64bit number.
//   constexpr auto number_range = 22;

//   // If we can perform an inpace write then attempt to do so.
//   if (size + number_range <= capacity) {
//     auto block_as_char = std::bit_cast<char*>(rented_block);
//     std::to_chars_result result = std::to_chars(
//         block_as_char + size, block_as_char + size + number_range, number);
//     if (result.ec != std::errc()) {
//       size += result.ptr - (block_as_char + size);
//     } else {
//       append('0'_byte);
//     }

//     return;
//   }

//   char working_buffer[number_range] = {};
//   std::to_chars_result result =
//       std::to_chars(working_buffer, working_buffer + number_range, number);
//   append(View::Bytes(reinterpret_cast<Byte*>(working_buffer),
//                      result.ptr - working_buffer));
// }

// auto Managed::Bytes::append(Long number) -> void {
//   // Largest size required to represent any 64bit number.
//   constexpr auto number_range = 22;

//   // If we can perform an inpace write then attempt to do so.
//   if (size + number_range <= capacity) {
//     auto block_as_char = std::bit_cast<char*>(rented_block);
//     std::to_chars_result result = std::to_chars(
//         block_as_char + size, block_as_char + size + number_range, number);
//     if (result.ec != std::errc()) {
//       size += result.ptr - (block_as_char + size);
//     } else {
//       append('0'_byte);
//     }

//     return;
//   }

//   char working_buffer[number_range] = {};
//   std::to_chars_result result =
//       std::to_chars(working_buffer, working_buffer + number_range, number);
//   append(View::Bytes(reinterpret_cast<Byte*>(working_buffer),
//                      result.ptr - working_buffer));
// }

// auto Managed::Bytes::append(Real_64 number) -> void {
//   // Largest size required to represent any 64bit number.
//   constexpr auto number_range = 28;

//   // If we can perform an inpace write then attempt to do so.
//   if (size + number_range > capacity) {
//     auto block_as_char = std::bit_cast<char*>(rented_block);
//     std::to_chars_result result = std::to_chars(
//         block_as_char + size, block_as_char + size + number_range, number);
//     if (result.ec != std::errc()) {
//       size += result.ptr - (block_as_char + size);
//     } else {
//       append('0'_byte);
//     }

//     return;
//   }

//   char working_buffer[number_range] = {};
//   std::to_chars_result result =
//       std::to_chars(working_buffer, working_buffer + number_range, number);
//   append(View::Bytes(reinterpret_cast<Byte*>(working_buffer),
//                      result.ptr - working_buffer));
// }

// namespace Perimortem::Memory::Managed {

// // A continous read / write buffer of bytes used for serialization.
// class Buffer {
//  public:
//   // Buffers can't be empty.
//   Buffer() = delete;

//   constexpr Buffer(Byte* source, Count range) {
//     size = range;
//     source_block = source;
//   }

//   template <Count N>
//   constexpr Buffer(Byte (&source)[N]) : source_block(&source), size(N) {}

//   inline constexpr auto get_size() const -> Count { return size; };
//   inline constexpr auto get_view() const -> View::Bytes {
//     return View::Bytes(source_block, size);
//   }

//   inline constexpr auto slice(Count start, Count size) const -> Buffer {
//     // Bit of a hack with std::min to avoid writing out of bounds.
//     // If the buffer points to nullptr the size will be zero which is always
//     // well defined behavior. We could avoid the min as the UB on nullptr or
//     // pointing out of range is capped by the buffer having a resulting size
//     of
//     // zero, but on if this ends up as a hot path in traces.
//     return Buffer(
//         source_block + std::min(start, size),
//         std::min(std::max(size, static_cast<Count>(0)), get_size() - start));
//   };

//   inline constexpr auto operator[](Count index) const -> Byte& {
//     return source_block[index];
//   };

//   //======================================================================
//   // Optimized operations
//   //======================================================================

//   auto serialize_count(Count number) -> void;
//   auto serialize_count(Count number) -> void;
//   auto serialize_number(Long number) -> void;
//   auto serialize_real64(Real_64 number) -> void;

//   auto store(Bits_64 data) -> void;
//   auto load(Bits_64 data) -> void;

//   // Writes bytes directly into the buffer.
//   template <typename storage_type,
//             std::endian source_order = std::endian::little>
//   auto serialize(View::Bytes bytes, Count location = 0) const -> Bool {
//     if (location + bytes.get_size() > size)
//       return false;

//     // When writing bytes we may not be aligned so we'll need to copy the
//     bytes. std::memcpy(source_block + location, bytes.get_data(),
//                 sizeof(storage_type));

//     return true;
//   }

//   // Writes bytes directly into the buffer.
//   template <typename storage_type,
//             std::endian source_order = std::endian::little>
//   auto write(storage_type data, Count location) const -> Bool {
//     if (location + sizeof(storage_type) > size)
//       return false;

//     if constexpr (std::endian::native != source_order) {
//       data = std::byteswap(data);
//     }

//     // When writing bytes we may not be aligned so we'll need to copy the
//     bytes. std::memcpy(source_block + location, data, sizeof(storage_type));

//     return true;
//   }

//   // Read with inplace incrementing counter.
//   template <typename storage_type,
//             std::endian source_order = std::endian::little>
//   auto incremental_write(storage_type data, Count& location) const -> Bool {
//     auto success = write<storage_type>(location);
//     if (success) {
//       location += sizeof(storage_type);
//     }

//     return data;
//   }

//  private:
//   Byte* source_block;
//   Count size;
// };

// }  // namespace Perimortem::Memory::Managed
