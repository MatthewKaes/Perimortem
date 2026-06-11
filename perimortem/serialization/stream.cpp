// // Perimortem Engine
// // Copyright © Matt Kaes

// #include "perimortem/serialization/stream.hpp"

// #include "perimortem/core/static/bytes.hpp"
// #include "perimortem/core/data.hpp"

// using namespace Perimortem;
// using namespace Perimortem::Core;
// using namespace Perimortem::Memory;

// constexpr auto one_byte_max = Bits_32(0x7F);
// constexpr auto two_byte_max = Bits_32(0x7FF);
// constexpr auto three_byte_max = Bits_32(0xFFFF);
// constexpr auto max_rune = Bits_32(0x10FFFF);

// constexpr auto two_byte_lead = Bits_8(0xC0);
// constexpr auto three_byte_lead = Bits_8(0xE0);
// constexpr auto four_byte_lead = Bits_8(0xF0);
// constexpr auto continuation = Bits_8(0x80);
// constexpr auto six_bit_mask = Bits_32(0x3F);

// static auto encode_rune(Bits_32 rune, Static::Bytes<4>& out) -> Count {
//   if (rune <= one_byte_max) {
//     out[0] = Bits_8(rune);
//     return 1;
//   }

//   if (rune <= two_byte_max) {
//     out[0] = Bits_8(two_byte_lead | Bits_8(rune >> 6));
//     out[1] = Bits_8(continuation | Bits_8(rune & six_bit_mask));
//     return 2;
//   }

//   if (rune <= three_byte_max) {
//     out[0] = Bits_8(three_byte_lead | Bits_8(rune >> 12));
//     out[1] = Bits_8(continuation | Bits_8((rune >> 6) & six_bit_mask));
//     out[2] = Bits_8(continuation | Bits_8(rune & six_bit_mask));
//     return 3;
//   }

//   if (rune <= max_rune) {
//     out[0] = Bits_8(four_byte_lead | Bits_8(rune >> 18));
//     out[1] = Bits_8(continuation | Bits_8((rune >> 12) & six_bit_mask));
//     out[2] = Bits_8(continuation | Bits_8((rune >> 6) & six_bit_mask));
//     out[3] = Bits_8(continuation | Bits_8(rune & six_bit_mask));
//     return 4;
//   }

//   return 0;
// }

// auto Perimortem::Serialization::Stream::Bytes::write(View::Bytes text) ->
// void {
//   storage.concat(text);
// }

// auto Perimortem::Serialization::Stream::Bytes::write(Bits_32 rune) -> void {
//   Static::Bytes<4> encoded;
//   const Count byte_count = encode_rune(rune, encoded);
//   storage.concat(encoded.get_view().slice(0, byte_count));
// }

// auto Perimortem::Serialization::Stream::Bytes::write_repeat(
//     Bits_32 rune,
//     Count n) -> void {
//   Static::Bytes<4> encoded;
//   const Count byte_count = encode_rune(rune, encoded);
//   if (byte_count == 0) [[unlikely]] {
//     return;
//   }

//   const Count start = storage.get_size();
//   storage.resize(start + byte_count * n);
//   Bits_8* dest = storage.get_access().get_data() + start;
//   for (Count i = 0; i < n; i++) {
//     Data::copy(dest + i * byte_count, encoded.get_data(), byte_count);
//   }
// }

// auto Perimortem::Serialization::Stream::Bytes::write_repeat(Byte b, Count n)
//     -> void {
//   const Count start = storage.get_size();
//   storage.resize(start + n);
//   Data::set(storage.get_access().get_data() + start, b, n);
// }

// auto Perimortem::Serialization::Stream::Bytes::get_cursor() const -> Cursor {
//   return Cursor(storage.get_view());
// }

// Perimortem::Serialization::Stream::Bytes::Cursor::Cursor(View::Bytes data)
//     : data(data), position(0) {}

// auto Perimortem::Serialization::Stream::Bytes::Cursor::read_rune() -> Bits_32
// {
//   if (is_done()) [[unlikely]] {
//     return Bits_32(0);
//   }

//   const Byte lead = data[position];

//   if ((lead & Bits_8(0x80)) == Bits_8(0)) {
//     position++;
//     return Bits_32(lead);
//   }

//   if ((lead & Bits_8(0xE0)) == two_byte_lead) {
//     if (position + 2 > data.get_size()) [[unlikely]] {
//       position++;
//       return Bits_32(0);
//     }
//     const Bits_32 rune = ((Bits_32(lead) & Bits_32(0x1F)) << 6) |
//                          (Bits_32(data[position + 1]) & six_bit_mask);
//     position += 2;
//     return rune;
//   }

//   if ((lead & Bits_8(0xF0)) == three_byte_lead) {
//     if (position + 3 > data.get_size()) [[unlikely]] {
//       position++;
//       return Bits_32(0);
//     }
//     const Bits_32 rune = ((Bits_32(lead) & Bits_32(0x0F)) << 12) |
//                          ((Bits_32(data[position + 1]) & six_bit_mask) << 6)
//                          | (Bits_32(data[position + 2]) & six_bit_mask);
//     position += 3;
//     return rune;
//   }

//   if ((lead & Bits_8(0xF8)) == four_byte_lead) {
//     if (position + 4 > data.get_size()) [[unlikely]] {
//       position++;
//       return Bits_32(0);
//     }
//     const Bits_32 rune = ((Bits_32(lead) & Bits_32(0x07)) << 18) |
//                          ((Bits_32(data[position + 1]) & six_bit_mask) << 12)
//                          |
//                          ((Bits_32(data[position + 2]) & six_bit_mask) << 6)
//                          | (Bits_32(data[position + 3]) & six_bit_mask);
//     position += 4;
//     return rune;
//   }

//   position++;
//   return Bits_32(0);
// }

// auto Perimortem::Serialization::Stream::Bytes::Cursor::is_done() const ->
// Bool {
//   return position >= data.get_size();
// }
