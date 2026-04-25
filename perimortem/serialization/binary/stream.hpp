// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/amorphous.hpp"
#include "perimortem/core/view/amorphous.hpp"

namespace Perimortem::Serialization::Binary {

class Stream {
 public:
  // For binary data we start with a continuation byte in UTF-8 along with the
  // version in the lower 6 bits.
  // This allows us to differentiate between ASCII and Unicode
  static constexpr Bits_8 identity_byte = 0xF5;

  enum class DataType : Byte {
    Unknown = 0,
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

  constexpr Stream(Core::Access::Amorphous data) : data(data) {};
  constexpr Stream(const Stream& rhs) : data(rhs.data) {};

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
  auto operator<<(const Core::View::Amorphous blob) -> Stream&;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }

 private:
  Core::Access::Amorphous data;
  Count ptr_location = 0;
  Bool valid_state = true;
};

}  // namespace Perimortem::Serialization::Binary
