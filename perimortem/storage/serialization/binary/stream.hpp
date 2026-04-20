// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Storage::Serialization::Binary {

class Stream {
 public:
  // For binary data we start with a continuation byte in UTF-8 along with the
  // version in the lower 6 bits.
  // This allows us to differentiate between ASCII and Unicode
  static constexpr Bits_8 identity_byte = 0xF5;

  enum class DataType : Byte {
    Bits_8 = identity_byte,
    Bits_16,
    Bits_32,
    Bits_64,
    SignedBits_8,
    SignedBits_16,
    SignedBits_32,
    SignedBits_64,
    Real_32,  // IEEE-754
    Real_64,  // IEEE-754
    Blob,
  };

  constexpr Stream(Memory::Dynamic::Bytes& source) : data(source) {};
  constexpr Stream(const Stream& rhs) : data(rhs.data) {};

  constexpr operator Memory::View::Bytes() const { return data.get_view(); }

  // Sets the location of the read/write pointer.
  // If the index is out of range then the pointer is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Bits_8 bin) -> Stream&;
  auto operator<<(const Bits_16 bin) -> Stream&;
  auto operator<<(const Bits_32 bin) -> Stream&;
  auto operator<<(const Bits_64 bin) -> Stream&;
  auto operator<<(const SignedBits_8 bin) -> Stream&;
  auto operator<<(const SignedBits_16 bin) -> Stream&;
  auto operator<<(const SignedBits_32 bin) -> Stream&;
  auto operator<<(const SignedBits_64 bin) -> Stream&;
  auto operator<<(const Real_32 bin) -> Stream&;
  auto operator<<(const Real_64 bin) -> Stream&;
  auto operator<<(const Memory::View::Bytes blob) -> Stream&;

  constexpr auto operator[](Count index) const -> Byte { return data[index]; }
  constexpr auto at(Count index) const -> Byte { return data[index]; }

  constexpr auto get_view() const -> const Memory::View::Bytes { return data; }
  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }

 private:
  Memory::Dynamic::Bytes& data;
  Count ptr_location = 0;
};

}  // namespace Perimortem::Storage::Serialization::Binary
