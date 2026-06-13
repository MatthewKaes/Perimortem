// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/base64.hpp"

#include <x86intrin.h>

#include "perimortem/core/bibliotheca.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

// Make it a bit easier to read which locations are invalid.
#define _ 0

// We truly can't have nice things... Still no proper support for c99
// initializers.
constexpr Bits_8 decode_lookup[256] = {
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  62, _,  _,  _,  63, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, _,  _,  _,  _,  _,  _,  _,  0,  1,  2,  3,  4,  5,  6,
  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, _,  _,  _,  _,  _,  _,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

constexpr Bits_8 encode_lookup[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static_assert(decode_lookup['A'] == 0);
static_assert(decode_lookup['B'] == 1);
static_assert(decode_lookup['Z'] == 25);
static_assert(decode_lookup['a'] == 26);
static_assert(decode_lookup['z'] == 51);
static_assert(decode_lookup['0'] == 52);
static_assert(decode_lookup['9'] == 61);
static_assert(decode_lookup['+'] == 62);
static_assert(decode_lookup['/'] == 63);
// Equals are dropped
static_assert(decode_lookup['='] == _);

// Extra bytes needed after the true output size for vectorized working space.
constexpr Count decode_underwrite_bytes = sizeof(__m128i) / 2;
constexpr Count decode_extra_bytes =
    sizeof(__m256i) / 4 + decode_underwrite_bytes;

auto vectorized_decode(Bits_8* text, View::Bytes source) -> Count {
  // On AMD processors that don't support AVX512 they "partially" supports it
  // using two fused AVX2 256bit buffers. To make sure we support just about
  // every modern CPU we can use two parallel AVX2 buffers unrolled. This is esp
  // useful as AMD Zen 5 seems to pick up on what we are trying to do and we
  // seem to get full "double pump aliased AVX512" performance without needing
  // to support a secondary code path.
  //
  // This provides AVX2 support for backcompat but allows most modern CPUs to
  // pipeline both commands at the same time. In testing the only downside is
  // the gathering phase and bitmask phases perform worse than native AVX512.
  //
  // UPDATE: Due to the "AndNot" optimization resulting in improved pipelining
  // and reducing the number of ymm registers we needed we can actually use 3
  // channels to maximize register usage.
  constexpr auto fused_channels = 3;
  constexpr auto avx2_channel_width = sizeof(__m256i);
  constexpr auto full_channel_width = avx2_channel_width * fused_channels;
  constexpr auto upper_lane_underwrite_buffer = decode_underwrite_bytes;
  constexpr Count output_stride = 3;
  constexpr Count source_stride = 4;

  // From the gather phase we write exactly 1 / 4 of an AVX2 channel in extra
  // zeros. The first channel's wastage is actually used by the second channel.
  // This means we need only 8 extra bytes compared to AVX512's 16, but the data
  // contention seems to be the main reason we don't get full double pump speeds
  // which lets us emulate a single AVX512 write with seemingly minimum cache
  // contention (at least on a 9950x3D).
  Count size = (source.get_size() / source_stride) * output_stride;

  // Construct source buffers.
  auto source_data = source.get_data();
  auto source_bytes = source.get_size();

  // Shrink to the correct size accounting for '=' padding.
  for (Count i = 0; i < 2; i++) {
    if (source[source.get_size() - 1 - i] != '=') {
      break;
    }
    size--;
    source_bytes--;
  }

  // Make sure we start at the end of the buffer space for vectorization.
  const auto vectorized_chunks = source.get_size() / full_channel_width;
  const auto output_vectorized_bytes =
      vectorized_chunks * full_channel_width * output_stride / source_stride;
  const auto source_vectorized_bytes = vectorized_chunks * full_channel_width;

  // Set the end location that we will write backwards from.
  auto output_stream = text + output_vectorized_bytes;
  source_data = source_data + source_vectorized_bytes;

  // Use AVX2 for vectorization as it's generally more available than AVX512
  // and has less throttling concerns, and this decision single handedly
  // complicates everything.
  //
  // For base 64 we really only have 5 ranges we care about, each of which can
  // be mapped to it's actual byte value with a simple offset:
  // A-Z: 65 -> -65
  // a-z: 97 -> -71
  // 0-9: 48 -> +4
  // +:   43 -> +19
  // /:   47 -> +16
  //
  // Great so all we have to do is map a character to an offset. There is most
  // likely a known way to do this mapping, but since I have "not invented
  // here" syndrome and insist on doing things the hard way, let's see if we
  // can pigeon hole each interesting range by shifting out a range.
  //
  // With AVX2 _mm256_shuffle_epi8 ONLY shuffles using 128-bit lanes meaning
  // we only get 16 values we can work with per lane. That means if we can
  // limit each look up by identifying a useful 4 bit range then we can do
  // look ups in a single shift. This is the problem with our early decision
  // to use AVX2 since if we had AVX512, not only would we get to use 256-bit
  // lanes, but it actually upgrades all the way the full 512-bit lane
  // offering us a 6 bit look up range.
  //
  // Given we expect valid ASCII, which is only a 7 bit format, we only
  // need to shave off 3-4 bits. Looking at the bits something interesting
  // stands out: If we shift off the 4 lower bits we get almost perfect bit
  // sets:
  //
  // '0' -> 0b_011____; -> 3
  // '9' -> 0b_011____; -> 3
  // 'A' -> 0b_100____; -> 4
  // 'Z' -> 0b_101____; -> 5
  // 'a' -> 0b_110____; -> 6
  // 'z' -> 0b_111____; -> 7
  // '+' -> 0b_010____; -> 2
  // '/' -> 0b_010____; -> 2
  //
  // Exactly 1 value conflicts so 3 bits just aren't enough to differentiate all
  // of the value ranges. It turns out Base64URL also doesn't help despite using
  // '_' and '-'. Instead we can take advantage of base64 only using ASCII chars
  // which leaves the MSB always 0.
  //
  // We can use the MSB by setting it _IF_ the lower 4 bits are exactly 0xF:
  //
  // '0' -> 0b_011____ -> 3
  // '9' -> 0b_011____ -> 3
  // 'A' -> 0b_100____ -> 4
  // 'O' -> 0b1100____ -> 12 now
  // 'Z' -> 0b_101____ -> 5
  // 'a' -> 0b_110____ -> 6
  // 'o' -> 0b1110____ -> 14 now
  // 'z' -> 0b_111____ -> 7
  // '+' -> 0b_010____ -> 2
  // '/' -> 0b1010____ -> 10
  //
  // This almost works but we can do slightly better. Rather than adding the
  // MSB we can instead negate all the bits and remove it with an AND mask:
  //
  // '0' -> 0b1100____ -> 12
  // '9' -> 0b1100____ -> 12
  // 'A' -> 0b1011____ -> 11
  // 'O' -> 0b0011____ -> 3 (top bit masked from 0xF invert mask)
  // 'Z' -> 0b1010____ -> 10
  // 'a' -> 0b1001____ -> 9
  // 'o' -> 0b0001____ -> 1 (top bit masked from 0xF invert mask)
  // 'z' -> 0b1000____ -> 8
  // '+' -> 0b1101____ -> 13
  // '/' -> 0b0101____ -> 5 (top bit masked from 0xF invert mask)
  //
  // The benefit of this is that 0xF is also clamped to a 16 byte range still
  // but we also ensure the control bit is also always disabled by the AND
  // mask with the same and manipulation, saving an entire "Or" instruction
  // which frees just enough resources to sneak in an additional round of
  // unrolling while we are blocked on latency for the data dependency.
  //
  // Now this is a bit of a code crime because unlike other vectorization
  // methods I haven't figured out a good way to to validate bad inputs.
  // For now it's undefined behavior, so it's on the caller to validate inputs
  // from untrusted sources if they aren't going to validate the output.
  const auto adjustment_values = _mm256_setr_epi8(
      // Lane 1
      /* 0 */ _,
      /* 'o' */ -71, _,
      /* 'O' */ -65, _,
      /* '/' */ +16, _, _,
      /* 'p' - 'z' */ -71,
      /* 'a' - 'n' */ -71,
      /* 'P' - 'Z' */ -65,
      /* 'A' - 'N' */ -65,
      /* 0 - 9 */ +4,
      /* + */ +19, _, _,

      // Lane 2 (same as Lane 1)
      /* 0 */ _,
      /* 'o' */ -71, 0,
      /* 'O' */ -65, 0,
      /* '/' */ +16, _, 0,
      /* 'p' - 'z' */ -71,
      /* 'a' - 'n' */ -71,
      /* 'P' - 'Z' */ -65,
      /* 'A' - 'N' */ -65,
      /* 0 - 9 */ +4,
      /* + */ +19, _, _);

  const auto invert_mask = _mm256_setr_epi8(
      // Lane 1 - Note the 0xF case removes an extra bit.
      0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111,
      0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b0111,

      // Lane 2 (same as Lane 1)
      0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111,
      0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b0111);
  const auto repack_shuffle = _mm256_setr_epi8(
      // Lane 1 is properly packed with left alignment.
      0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x01, 0x00, 0x06, 0x05, 0x04, 0x0A, 0x09,
      0x08, 0x0E, 0x0D, 0x0C,

      // Lane 2 is packed to the right to allow for underwriting with no extra
      // space left at the end.
      0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x01, 0x00, 0x06, 0x05, 0x04, 0x0A, 0x09,
      0x08, 0x0E, 0x0D, 0x0C);

  // Write both the chunks and the pipelined ymm groups in reverse order to
  // support the underwriting process.
  for (auto inverse_chunk = vectorized_chunks; inverse_chunk > 0;
       inverse_chunk--) {
    // Linear accounting
    output_stream -=
        (avx2_channel_width - avx2_channel_width / 4) * fused_channels;
    source_data -= full_channel_width;

    for (auto ymm = fused_channels - 1; ymm >= 0; ymm--) {
      // Load
      const auto channel_value = _mm256_loadu_si256(
          Data::cast<const __m256i>(source_data + sizeof(__m256i) * ymm));

      // Bit hacking the lookup index
      const auto shift = _mm256_srli_epi64(channel_value, 4);
      // Always valid for ASCII as the MSB control bit is always 0.
      const auto nibble_mask = _mm256_shuffle_epi8(invert_mask, channel_value);
      // AndNot for the speed up that ensures we have a valid shuffle mask
      // while also performing the out of range clip.
      const auto adjusted_index = _mm256_andnot_si256(shift, nibble_mask);

      // Convert to the actual 6 bit value.
      const auto adjust_value =
          _mm256_shuffle_epi8(adjustment_values, adjusted_index);
      const auto bit_value = _mm256_add_epi8(adjust_value, channel_value);

      // Lane merging:
      //
      // AAAAAA__ BBBBBB__ CCCCCC__ DDDDDD__
      // Multiply horizontally at 16 bit boundries using 64 to shift 6 bits.
      // AAAAAA__ * 1  = AAAAAA__ ________
      // bbbbbb__ * 64 = ______bb bbbb____
      // Add multiplications:
      // AAAAAAbb bbbb____ CCCCCCdd dddd____
      const __m256i multiply_by_64 = _mm256_set_epi64x(
          0x0140014001400140, 0x0140014001400140, 0x0140014001400140,
          0x0140014001400140);
      const __m256i multiply_by_4096 = _mm256_set_epi64x(
          0x0001100000011000, 0x0001100000011000, 0x0001100000011000,
          0x0001100000011000);
      const auto half_merged = _mm256_maddubs_epi16(bit_value, multiply_by_64);

      // Now do the same thing horizontally across 32 bit boundries which
      // gives us a clean stopping point.
      const auto merged_i32 = _mm256_madd_epi16(half_merged, multiply_by_4096);
      const auto lane_repack = _mm256_shuffle_epi8(merged_i32, repack_shuffle);

      // Rather than premuteevar8x32_epi32 and a store we can just do double
      // lane store. This saves the lane_shuffle and ymm_repack taking up
      // registers.
      //
      // As an additional optimization we can then also store the upper lane
      // by under writing the lower lane by 8 bytes, filling the gap created
      // by the lane repack shuffle.
      //
      // This aligns the upper bytes without needing _mm256_extractf128_si256,
      // which is super slow.
      _mm256_storeu_si256(
          (__m256i*)(output_stream + 24 * ymm - upper_lane_underwrite_buffer),
          lane_repack);

      // Store lower lane directly at the correct position.
      const auto lower_lane = _mm256_castsi256_si128(lane_repack);
      _mm_storeu_si128(
          (__m128i*)(output_stream + 24 * ymm -
                     upper_lane_underwrite_buffer / 2),
          lower_lane);
    };
  }

  // Reset the output location for the scalar process
  output_stream = text + output_vectorized_bytes;
  source_data = source_data + source_vectorized_bytes;
  source_bytes -= source_vectorized_bytes;

  for (Count i = 0, j = 0; j < source_bytes;
       i += output_stride, j += source_stride) {
    output_stream[i] = (decode_lookup[source_data[j]] << 2) |
                       ((decode_lookup[source_data[j + 1]] & 0x30) >> 4);
    output_stream[i + 1] = ((decode_lookup[source_data[j + 1]] & 0x0f) << 4) |
                           ((decode_lookup[source_data[j + 2]] & 0x3c) >> 2);
    output_stream[i + 2] = ((decode_lookup[source_data[j + 2]] & 0x03) << 6) |
                           decode_lookup[source_data[j + 3]];
  }

  return size;
}

auto vectorize_encode(Access::Bytes output, View::Bytes source) -> void {
  // On AMD processors that don't support AVX512 they "partially" supports it
  // using two fused AVX2 256bit buffers. To make sure we support just about
  // every modern CPU we can use two parallel AVX2 buffers unrolled. This is esp
  // useful as AMD Zen 5 seems to pick up on what we are trying to do and we
  // seem to get full "double pump aliased AVX512" performance without needing
  // to support a secondary code path.
  //
  // This provides AVX2 support for backcompat but allows most modern CPUs to
  // pipeline both commands at the same time. In testing the only downside is
  // the gathering phase and bitmask phases perform worse than native AVX512.
  constexpr auto fused_channels = 2;
  constexpr auto avx2_channel_width = sizeof(__m256i);
  constexpr auto full_channel_width = avx2_channel_width * fused_channels;
  constexpr Count output_stride = 4;
  constexpr Count source_stride = 3;
  constexpr auto encode_filter = 64 - 1;

  auto source_data = source.get_data();
  auto source_bytes = source.get_size();
  const Count left_over = source_bytes % 3;
  auto output_stream = output.get_data();

  // If the vector is long enough start by doing a scalar pass so we have room
  // to underread the buffer, regardless of its source.
  if (source_bytes > full_channel_width * 2) {
    // Since AVX2 can't shuffle around 128 bit lanes we need to do two scaler
    // chunks to allow us to do an offset read safely.
    //
    // We could technically avoid this step if we knew the view always came from
    // the Bibliotheca but since it can come form any source we process 2 chunks
    // to give us room.
    for (Count i = 0; i < 2; i++) {
      output_stream[0] = encode_lookup[(source_data[0] >> 2)];
      output_stream[1] = encode_lookup
          [((source_data[0] & 0b00000011) << 4) | (source_data[1] >> 4)];
      output_stream[2] = encode_lookup
          [((source_data[1] & 0b00001111) << 2) | (source_data[2] >> 6)];
      output_stream[3] = encode_lookup[source_data[2] & encode_filter];

      source_bytes -= 3;
      output_stream += output_stride;
      source_data += source_stride;
    }
  }

  // Alrignt now that we have our 6 bit values we need to pigeon hole them since
  // we only have 4 bits worth of lookup.
  //
  // For base 64 we really only have 5 ranges we care about and all the offsets
  // are just the inverse of the decode version:
  // A-Z: 65 -> +65
  // a-z: 97 -> +71
  // 0-9: 48 -> -4
  // +:   43 -> -19
  // /:   47 -> -16
  //
  // The bit ranges we have to deal with however are a bit more annoying:
  // 000000 -> +65
  // 011001 -> +65
  // 011010 -> +71
  // 110011 -> +71
  // 110100 -> -4
  // 111101 -> -4
  // 111110 -> -19
  // 111111 -> -16
  //
  // Since we have the benefit of continous ranges this time we can look at
  // compressing the alpha range into two checks to save a majority of values.
  //
  // If we subtract 51 then we can compress the alphas into a single value as
  // long as we saturate the result to 0:
  // __0000 -> "Alpha" -> +71
  // __0001 -> '0' -> -4
  // __0010 -> '1' -> -4
  // __0011 -> '2' -> -4
  // __0100 -> '3' -> -4
  // __0101 -> '4' -> -4
  // __0110 -> '5' -> -4
  // __0111 -> '6' -> -4
  // __1000 -> '7' -> -4
  // __1001 -> '8' -> -4
  // __1010 -> '9' -> -4
  // __1011 -> '+' -> -19
  // __1100 -> '/' -> -16
  // __1101 -> ???
  // __1110 -> ???
  // __1111 -> ???
  //
  // A CMPGT can let us compute an additional offset for the lower alpha values.
  // With that we can conditionally sub 6;
  const auto adjustment_values = _mm256_setr_epi8(
      // Lane 1
      /* Alphas */ 71,
      /* '0' */ -4,
      /* '1' */ -4,
      /* '2' */ -4,
      /* '3' */ -4,
      /* '4' */ -4,
      /* '5' */ -4,
      /* '6' */ -4,
      /* '7' */ -4,
      /* '8' */ -4,
      /* '9' */ -4,
      /* '+' */ -19,
      /* '/' */ -16, _, _, _,

      // Lane 2 (same as Lane 1)
      /* Alphas */ 71,
      /* '0' */ -4,
      /* '1' */ -4,
      /* '2' */ -4,
      /* '3' */ -4,
      /* '4' */ -4,
      /* '5' */ -4,
      /* '6' */ -4,
      /* '7' */ -4,
      /* '8' */ -4,
      /* '9' */ -4,
      /* '+' */ -19,
      /* '/' */ -16, _, _, _);

  // Split each 3 byte chunk into it's four 6 bit components.
  // Using a shuffle lets us both swap bytes into the correct little endian
  // format as well as duplicate the 4th 6bit block all in one go.
  constexpr auto a_index = 0;
  constexpr auto b_index = 1;
  constexpr auto c_index = 1;
  constexpr auto d_index = 2;
  const auto byte_shuffle = _mm256_set_epi8(
      // 8th block
      25 + c_index, 25 + d_index, 25 + a_index, 25 + b_index,
      // 7th block
      22 + c_index, 22 + d_index, 22 + a_index, 22 + b_index,
      // 6th block
      19 + c_index, 19 + d_index, 19 + a_index, 19 + b_index,
      // 5th block
      16 + c_index, 16 + d_index, 16 + a_index, 16 + b_index,
      // 4th block
      13 + c_index, 13 + d_index, 13 + a_index, 13 + b_index,
      // 3rd block
      10 + c_index, 10 + d_index, 10 + a_index, 10 + b_index,
      // 2nd block
      7 + c_index, 7 + d_index, 7 + a_index, 7 + b_index,
      // 1st block
      4 + c_index, 4 + d_index, 4 + a_index, 4 + b_index);

  constexpr auto input_bytes_per_channel = sizeof(Bits_32) * 6;
  constexpr auto input_bytes_per_iteration =
      input_bytes_per_channel * fused_channels;

  constexpr auto output_bytes_per_channel = sizeof(Bits_32) * 8;
  constexpr auto output_bytes_per_iteration =
      output_bytes_per_channel * fused_channels;

  // Need to be careful about reading past the end.
  while (source_bytes > input_bytes_per_iteration + 4) {
    for (auto ymm = fused_channels - 1; ymm >= 0; ymm--) {
      // I assume this trick is used else where, but if we under read by 4
      // bytes we can load 6 chunks of data split on the lane boundary.
      //
      //  32   32   32   32  |  32   32   32   32
      // ____ AAAB BBCC CDDD | EEEF FFGG GHHH ____
      const auto channel_value = _mm256_loadu_si256(
          Data::cast<const __m256i>(
              source_data - 4 + ymm * input_bytes_per_channel));

      const auto chunk_data = _mm256_shuffle_epi8(channel_value, byte_shuffle);

      // Extract the c and a components out of each shuffled 4 byte chunk.
      const auto c_and_a = _mm256_and_si256(
          chunk_data, _mm256_set1_epi32(0b00001111'11000000'11111100'00000000));
      const auto d_and_b = _mm256_and_si256(
          chunk_data, _mm256_set1_epi32(0b00000000'00111111'00000011'11110000));

      // We don't have a good way to do variable length "shift right", so we can
      // "shift left" using mul and then just take the high 16 bytes.
      // Shift C right 6  -> (C << 10) >> 16
      // Shift A right 10 -> (A <<  6) >> 16
      const auto low_bytes = _mm256_mulhi_epu16(
          c_and_a, _mm256_set1_epi32(0b0000010000000000'0000000001000000));
      // For D and B we can just do a variable length "shift left" in place.
      // Shift B left 4 -> (B << 4) >> 0
      // Shift A left 8 -> (D << 8) >> 0
      const auto high_bytes = _mm256_mullo_epi16(
          d_and_b, _mm256_set1_epi32(0b0000000100000000'0000000000010000));

      const auto final_bit_values = _mm256_or_si256(low_bytes, high_bytes);

      // Caculate the offset and filter lower alpha values.
      const auto shuffle_indexes =
          _mm256_subs_epu8(final_bit_values, _mm256_set1_epi8(51));
      const auto lower_alpha_mask =
          _mm256_cmpgt_epi8(_mm256_set1_epi8(26), final_bit_values);
      const auto lower_alpha_offset =
          _mm256_and_si256(_mm256_set1_epi8(-6), lower_alpha_mask);

      const auto offset_values =
          _mm256_shuffle_epi8(adjustment_values, shuffle_indexes);
      const auto final_values = _mm256_add_epi8(
          _mm256_add_epi8(final_bit_values, offset_values), lower_alpha_offset);

      _mm256_storeu_si256(
          Data::cast<__m256i>(output_stream + ymm * output_bytes_per_channel),
          final_values);
    }

    source_bytes -= input_bytes_per_iteration;
    source_data += input_bytes_per_iteration;
    output_stream += output_bytes_per_iteration;
  }

  // Scalar fallback: >= rather than > so the final complete 3-byte group is
  // not skipped when source size is an exact multiple of 3.
  while (source_bytes >= source_stride) {
    output_stream[0] = encode_lookup[(source_data[0] >> 2)];
    output_stream[1] = encode_lookup
        [((source_data[0] & 0b00000011) << 4) | (source_data[1] >> 4)];
    output_stream[2] = encode_lookup
        [((source_data[1] & 0b00001111) << 2) | (source_data[2] >> 6)];
    output_stream[3] = encode_lookup[source_data[2] & encode_filter];

    source_bytes -= 3;
    output_stream += output_stride;
    source_data += source_stride;
  }

  // Left over padding.
  switch (left_over) {
  case 1:
    output_stream[0] = encode_lookup[(source_data[0] >> 2)];
    output_stream[1] = encode_lookup[((source_data[0] & 0b00000011) << 4)];
    output_stream[2] = '=';
    output_stream[3] = '=';
    break;
  case 2:
    output_stream[0] = encode_lookup[(source_data[0] >> 2)];
    output_stream[1] = encode_lookup
        [((source_data[0] & 0b00000011) << 4) | (source_data[1] >> 4)];
    output_stream[2] = encode_lookup[((source_data[1] & 0b00001111) << 2)];
    output_stream[3] = '=';
    break;
  default:
    break;
  }
}

auto Base64::decode(View::Bytes source) -> Dynamic::Bytes {
  // If empty than avoid any allocations or vectorization and just return an
  // empty optimized Dynamic::Bytes if the size isn't large enough.
  if (source.get_size() < 3) {
    return Dynamic::Bytes();
  }

  Count size = (source.get_size() / 4) * 3;

  // Allocate the bytes with the full size + working buffer.
  Memory::Dynamic::Bytes bytes;
  bytes.forgetful_resize(size + decode_extra_bytes);

  // The algorithm uses decode_underwrite_bytes of the Bibliotheca's guaranteed
  // underwrite region before the allocation pointer.
  static_assert(
      decode_underwrite_bytes <= Bibliotheca::legal_underwrite_size,
      "Base64 decode requires more underwrite bytes than are guaranteed by the "
      "Bibliotheca, ensure Perimortem is configured correctly");

  Bits_8* text = bytes.get_access().get_data();
  Count actual_size = vectorized_decode(text, source);
  bytes.resize(actual_size);
  return bytes;
}

auto Base64::decode(Allocator::Arena& arena, View::Bytes source)
    -> View::Bytes {
  if (source.get_size() < 3) {
    return View::Bytes();
  }

  Count size = (source.get_size() / 4) * 3;

  // Pre-pad by decode_underwrite_bytes so the vectorized loop can underwrite.
  Bits_8* text =
      arena.allocate(decode_underwrite_bytes + size + decode_extra_bytes) +
      decode_underwrite_bytes;
  Count actual_size = vectorized_decode(text, source);
  return View::Bytes(text, actual_size);
}

auto Base64::encode(View::Bytes source) -> Dynamic::Bytes {
  if (source.is_empty()) {
    return Dynamic::Bytes();
  }

  const Count left_over = source.get_size() % 3;
  const Count output_size = (source.get_size() / 3) * 4 + (left_over ? 4 : 0);
  Dynamic::Bytes bytes;
  bytes.forgetful_resize(output_size);
  vectorize_encode(bytes.get_access(), source);
  return bytes;
}

auto Base64::encode(Allocator::Arena& arena, View::Bytes source)
    -> View::Bytes {
  if (source.is_empty()) {
    return View::Bytes();
  }

  const Count left_over = source.get_size() % 3;
  const Count output_size = (source.get_size() / 3) * 4 + (left_over ? 4 : 0);
  Bits_8* output = arena.allocate(output_size);
  vectorize_encode(Access::Bytes(output, output_size), source);
  return View::Bytes(output, output_size);
}
