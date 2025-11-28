// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/formats/base64.hpp"

#include <x86intrin.h>

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Base64;

// We truly can't have nice things... Still no proper support for c99
// initializers.
static constexpr uint32_t decode_lookup[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  62, 0,  0,  0,  63, 52, 53, 54, 55, 56, 57,
    58, 59, 60, 61, 0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 0,  0,  0,  0,  0,  0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

static_assert(decode_lookup['A'] == 0);
static_assert(decode_lookup['B'] == 1);
static_assert(decode_lookup['Z'] == 25);
static_assert(decode_lookup['a'] == 26);
static_assert(decode_lookup['z'] == 51);
static_assert(decode_lookup['0'] == 52);
static_assert(decode_lookup['9'] == 61);
static_assert(decode_lookup['+'] == 62);
static_assert(decode_lookup['/'] == 63);
static_assert(decode_lookup['='] == 0);

Decoded::Decoded(const Memory::ManagedString& source,
                 bool disable_vectorization) {
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
  constexpr const auto fused_channels = 3;
  constexpr const auto avx2_channel_width = sizeof(__m256i);
  constexpr const auto full_channel_width = avx2_channel_width * fused_channels;

  // From the gather phase we write exactly 1 / 4 of an AVX2 channel in extra
  // zeros. The first channel's wastage is actually used by the second channel.
  // This means we need only 8 extra bytes compared to AVX512's 16, but the data
  // contention seems to be the main reason we don't get full double pump speeds
  // which lets us emulate a single AVX512 write with seemingly minimum cache
  // contention (at least on a 9950x3D).
  size = source.get_size() / 4 * 3;
  rented_block = Bibliotheca::check_out(size + avx2_channel_width / 4);
  text = reinterpret_cast<char*>(Bibliotheca::preface_to_corpus(rented_block));

  // Construct source buffers.
  const auto source_view = source.get_view();
  auto source_data = reinterpret_cast<const uint8_t*>(source_view.data());
  auto source_bytes = source_view.size();
  auto output_stream = text;

  // Shrink to the correct size.
  // We don't have to worry about writting extra zeros as we always request
  // extra padding from the Bibliotheca.
  for (uint32_t i = 0; i < 2; i++) {
    if (source[source.get_size() - 1 - i] != '=') {
      break;
    }

    size--;
    source_bytes--;
  }

  if (!disable_vectorization) {
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
    // stands out: If we shift off the 4 lower bits we get a perfect 4 bit set!
    // '0' -> 0b_011____; -> 3
    // '9' -> 0b_011____; -> 3
    // 'A' -> 0b_100____; -> 4
    // 'Z' -> 0b_101____; -> 5
    // 'a' -> 0b_110____; -> 6
    // 'z' -> 0b_111____; -> 7
    // '+' -> 0b_010____; -> 2
    // '/' -> 0b_010____; -> 2
    //
    // Wait... Exactly 1 value conflicts. It seems 3 bits just aren't enough
    // to differentiate all of the value ranges. It turns out Base64URL also
    // doesn't help despite using '_' and '-'. It a good thing ASCII leaves that
    // MSB completely untouched and unware of our intentions.
    //
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠋⣤⠉⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢀⣾⣿⣿⣦⣀⠛⣿⣿⣿⣿⣿⣿⣿  Me ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢀⣿⣿⣿⣿⣿⣿⣷⣄⠛⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⠷⠈⠻⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠛⠉⢉⠀⠙⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⣾⢿⡿⢝⣿⣿⣿⣿⣧⣤⣶⣾⣾⣿⣿⣶⣤⣄⠀⠙⠛⠿⠛⠋⣉⣤⣤⣶⣾⣿⣿⣿⣿⣿⠈⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⢸⠛⢁⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠀⣀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⢠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠏⣴⣿⣿⡿⠁⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⣾⣿⣿⡟⠀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⣰⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⣾⣿⣿⡿⢀⣾⣿⣿⣿⣿⣿⠟⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⣿⣿⣿⣿⡿⠁⣠⠙⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⠃⣾⣿⣿⣿⠀⣾⠟⠀⠘⠛⠿⠏⣰⡄⢻⣿⣿⣿⣿⣿⠿⢿⣿⣿⡟⢉⢻⣿⣿⣿⣿⡆⣹⣿⣿⣿⣿⣿⣤⣿⢸⡆⠹⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⠃⢠⣿⣿⣿⠇⠘⢠⣾⣿⣿⣿⣿⣶⣤⠙⠀⢻⣿⣿⠟⣀⣾⠀⡿⠃⣴⣿⡀⠻⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⡞⣿⠀⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⠀⣼⣿⣿⣿⣦⡄⢿⣿⣿⡇⠈⣿⣿⣿⣿⡇⠀⠿⢀⡾⢉⣤⣤⣤⣶⣶⣤⣉⠀⢹⣿⣿⠁⣿⣿⣿⣿⠀⣿⣿⣿⣷⣿⣇⢸⣿
    // ⣿⣿⣿⣿⣿⣿⣿⠠⠛⢛⡋⢹⣿⣿⣄⠙⣿⡗⠀⠸⣿⣿⣿⠏⣿⣶⡿⢰⣿⣿⣿⠿⣿⣿⣿⣿⣿⣀⠙⣿⠀⣿⣿⣿⣿⡄⢸⣿⣿⣿⢿⣿⠘⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣶⣿⣿⡇⢸⣿⣿⡇⣿⣶⣄⠀⠀⠛⠉⣀⣶⣿⣿⣇⢸⣿⣿⣿⠀⠘⣿⣿⣿⣿⣿⡄⠁⣼⣿⣿⣿⣿⣿⠀⡿⣿⡧⠀⣿⠀⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠈⣿⡿⠣⠈⠻⣿⣿⣿⣿⣿⡉⣿⠿⣿⣿⣦⣀⠻⢿⡆⠀⢻⣿⣿⣿⣿⡿⣰⣿⣿⣿⣿⣿⣿⠄⢿⡹⡇⠀⠈⢀⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⡿⠀⠀⢿⣦⣀⠉⠛⠿⣿⣿⣀⣴⡀⢀⣿⣿⣿⣶⣤⣤⣈⠉⠉⠉⣠⡴⢻⣿⣿⣿⣿⣿⣿⡇⢸⣷⡏⠀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣀⣿⣷⠀⠀⣄⢹⣿⡶⠀⣀⠉⠙⠛⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣧⠠⣾⣿⠀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠃⠀⣤⡤⠁⣠⣶⣿⣿⣿⣿⣶⣴⣤⣀⣀⠉⠉⠉⠉⠉⢉⠀⠀⣺⣿⣿⣿⣿⣿⣿⣷⠀⠘⢿⠀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠃⣼⣿⣿⠀⣼⣿⣿⠋⣹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⠟⠉⠛⣿⣿⣿⢠⣷⣀⣀⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣿⣿⣿⣄⠉⠋⡀⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠻⣿⣿⣿⠀⠋⢀⣾⣿⣿⣆⠈⠏⢨⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⠙⢿⣿⣿⣿⠛⠀⠙⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠄⠙⣿⣿⣿⣿⠀⣄⠉⣿⣿⣿⣦⣾⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⣄⣀⣠⠂⡨⢑⢀⠈⠻⠿⣿⣿⣿⣿⣿⠿⠛⠁⠘⠀⠛⠿⠟⢀⣿⣿⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠀⡅⠠⠨⠨⠐⠠⢄⡀⠠⠄⠠⢰⠀⠡⠘⠀⣿⣶⣾⣿⣿⣿⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿    Code   ⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀⠪⢈⢈⠠⢈⠨⢈⢀⢈⢈⢐⠀⠌⠀⡆⢿⣿⣿⣿⣿⠟⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿   Crimes  ⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠁⠂⠢⠢⠰⠔⠐⠐⠈⠁⠀⡉⢿⣦⣀⡈⠉⣀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠀⠀⠀⠀⠀⣷⣶⣤⣤⡀⠀⠀⠀⠀⢠⣷⣀⣀⣤⠈⣻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⡀⢻⣿⣿⣿⣿⣿⣿⡁⠀⠀⠀⢀⣾⣿⣿⣿⣿⠁⠀⠀⠀⠀⣎⠛⠻⠾⠋⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⠟⢁⣴⣿⣿⣦⠙⣿⣿⣿⣿⣿⣿⣶⣶⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⢠⣿⣿⣿⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⠿⠁⣠⣾⣿⣿⣿⣿⠋⢠⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣤⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⠋⣀⣴⣿⣿⣿⣿⠿⠛⢠⣶⣻⢧⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣆⠻⣿⣿⣿⠟⠉⣤⣶⣷⡀⠻⣾⠟⠁⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣄⠙⠉⣠⣾⣿⣿⣿⣿⣿⣶⡀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    // ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    //
    // Let's just go ahead and set the MSB _IF_ the lower 4 bits are exactly
    // 0xF:
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
    // but we also ensure the control bit is also always disabled by the and
    // mask with the same and manipulation saving an entire "Or" instruction and
    // frees up one ymm register that was previous used for a nibble mask.
    //
    // Now the reason this is a code crime is because unlike other vectorization
    // methods I haven't figured out a good way to to validate bad inputs. To
    // fix that problem we will take a number out of C++'s book and just define
    // bad input as undefined behavior in the name of performance and call it a
    // day. If you want data guarantees then use the slower version.
    const __m256i adjustment_values = _mm256_setr_epi8(
        // Lane 1
        0, /* 'o' */ -71, 0, /* 'O' */ -65, 0, /* '/' */ +16, 0, 0,
        /* 'p' - 'z' */ -71, /* 'a' - 'n' */ -71, /* 'P' - 'Z' */ -65,
        /* 'A' - 'N' */ -65, /* 0 - 9 */ +4, /* + */ +19, 0, 0,

        // Lane 2 (same as Lane 1)
        0, /* 'o' */ -71, 0, /* 'O' */ -65, 0, /* '/' */ +16, 0, 0,
        /* 'p' - 'z' */ -71, /* 'a' - 'n' */ -71, /* 'P' - 'Z' */ -65,
        /* 'A' - 'N' */ -65, /* 0 - 9 */ +4, /* + */ +19, 0, 0);

    const __m256i invert_mask = _mm256_setr_epi8(
        // Lane 1 - Note the 0xF case removes an extra bit.
        0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111,
        0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111,

        // Lane 2 (same as Lane 1)
        0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111,
        0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b1111, 0b0111);
    const __m256i repack_shuffle = _mm256_setr_epi8(
        0x02, 0x01, 0x00, 0x06, 0x05, 0x04, 0x0A, 0x09, 0x08, 0x0E, 0x0D, 0x0C,
        0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x01, 0x00, 0x06, 0x05, 0x04, 0x0A, 0x09,
        0x08, 0x0E, 0x0D, 0x0C, 0xFF, 0xFF, 0xFF, 0xFF);
    // const __m256i lane_shuffle =
    //     _mm256_setr_epi32(0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0xFF, 0xFF);

    // Only run on larger chunks.
    while (source_bytes >= full_channel_width) {
      for (int ymm = 0; ymm < fused_channels; ymm++) {
        // Load
        const auto channel_value =
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(
                source_data + sizeof(__m256i) * ymm));

        // Bit hacking the lookup index
        const auto shift = _mm256_srli_epi64(channel_value, 4);
        // Always valid for ASCII as the MSB control bit is always 0.
        const auto nibble_mask =
            _mm256_shuffle_epi8(invert_mask, channel_value);
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
        const __m256i multiply_by_64 = _mm256_set1_epi32(0x01400140);
        const __m256i multiply_by_4096 = _mm256_set1_epi32(0x00011000);
        const auto half_merged =
            _mm256_maddubs_epi16(bit_value, multiply_by_64);

        // Now do the same thing horizontally across 32 bit boundries which
        // gives us a clean stopping point.
        const auto merged_i32 =
            _mm256_madd_epi16(half_merged, multiply_by_4096);
        const auto lane_repack =
            _mm256_shuffle_epi8(merged_i32, repack_shuffle);

        // Rather than premuteevar8x32_epi32 and a store we can just do double
        // lane store. This saves the lane_shuffle and ymm_repack taking up
        // registers.

        // Store lower lane directly.
        const auto lower_lane = _mm256_castsi256_si128(lane_repack);
        _mm_storeu_si128((__m128i*)(output_stream + 24 * ymm), lower_lane);

        // Store upper lane offset by 12. This overwrites the 4 empty bytes from
        // the first lane.
        const auto upper_lane = _mm256_extractf128_si256(lane_repack, 1);
        _mm_storeu_si128((__m128i*)(output_stream + 24 * ymm + 12), upper_lane);
      };

      // Linear accounting
      output_stream +=
          (avx2_channel_width - avx2_channel_width / 4) * fused_channels;
      source_bytes -= full_channel_width;
      source_data += full_channel_width;
    }
  }

  // Accumulate any remaining bytes in either mode.
  constexpr uint32_t output_stride = 3;
  constexpr uint32_t source_stride = 4;
  for (uint32_t i = 0, j = 0; j < source_bytes;
       i += output_stride, j += source_stride) {
    output_stream[i] = (decode_lookup[source_data[j]] << 2) |
                       ((decode_lookup[source_data[j + 1]] & 0x30) >> 4);
    output_stream[i + 1] = ((decode_lookup[source_data[j + 1]] & 0x0f) << 4) |
                           ((decode_lookup[source_data[j + 2]] & 0x3c) >> 2);
    output_stream[i + 2] = ((decode_lookup[source_data[j + 2]] & 0x03) << 6) |
                           decode_lookup[source_data[j + 3]];
  }
}

Decoded::Decoded(const Decoded& rhs) {
  text = rhs.text;
  size = rhs.size;
  rented_block = rhs.rented_block;
  Bibliotheca::reserve(rented_block);
}

Decoded::~Decoded() {
  Bibliotheca::remit(rented_block);
}