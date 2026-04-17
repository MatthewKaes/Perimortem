// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/uuid.hpp"
#include "perimortem/system/random.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::System;

static constexpr Byte hex_lookup[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

// Just keep it simple and fix whenever we need to compile with GCC.
static const Byte hexTable[256] = {
    ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,  ['5'] = 5,
    ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,  ['A'] = 10, ['B'] = 11,
    ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15, ['a'] = 10, ['b'] = 11,
    ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15};

constexpr auto convert_hex_buffer(const Static::Bytes<16>& hex_bytes)
    -> Bits_64 {
  Bits_64 result = 0;
  for (Count i = 0; i < hex_bytes.get_size(); i++) {
    result |= hex_bytes[i] << (60UL - i * 4);
  }
  return result;
}

Uuid::Uuid() {}

Uuid::Uuid(Static::Vector<Bits_64, 2> source) : value(source) {}

Uuid::Uuid(Static::Bytes<36> source) {
  deserialize(source);
}

auto Uuid::operator==(const Uuid& rhs) const -> bool {
  return value[0] == rhs.value[0] && value[1] == rhs.value[1];
}

auto Uuid::deserialize(const Static::Bytes<36>& uuid_string) -> Uuid& {
  // RFC-4122 spec:
  // 8-4-4-4-12
  value[0] = convert_hex_buffer({{
      uuid_string[0],
      uuid_string[1],
      uuid_string[2],
      uuid_string[3],
      uuid_string[4],
      uuid_string[5],
      uuid_string[6],
      uuid_string[7],
      // -
      uuid_string[9],
      uuid_string[10],
      uuid_string[11],
      uuid_string[12],
      // -
      uuid_string[14],
      uuid_string[15],
      uuid_string[16],
      uuid_string[17],
  }});
  value[1] = convert_hex_buffer({{
      uuid_string[19],
      uuid_string[20],
      uuid_string[21],
      uuid_string[22],
      // -
      uuid_string[24],
      uuid_string[25],
      uuid_string[26],
      uuid_string[27],
      uuid_string[28],
      uuid_string[29],
      uuid_string[30],
      uuid_string[31],
      uuid_string[32],
      uuid_string[33],
      uuid_string[34],
      uuid_string[35],
  }});

  return *this;
}

auto Uuid::serialize() const -> Static::Bytes<36> {}

auto Uuid::generate() -> Uuid {
  Bits_64 values[] = {System::Random::generate(), System::Random::generate()};
  return Uuid(values);
}
