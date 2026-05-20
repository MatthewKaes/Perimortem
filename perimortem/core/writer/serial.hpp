// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Writer {

// Writes self-describing values into a byte buffer using a compact tagged
// format that Reader::Serial can decode without any out-of-band schema.
//
// Each scalar value is preceded by a one-byte type tag from the DataType enum.
// Blob values additionally carry an element-type tag and a run-length-encoded
// count: the count is stored using the smallest integer type that fits (Bits_8
// for ≤255 elements, Bits_16 for ≤65535, and so on). All multi-byte payloads
// are stored in little-endian order.
//
// On overflow the writer enters an invalid state and subsequent writes are
// silently discarded.
class Serial {
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

  // Defaults to little endian.
  // To avoid bugs Serial streams don't default or support "native".
  enum class Endian : Byte {
    Little,
    Big,
  };

  constexpr Serial(Core::Access::Bytes data) : data(data) {};
  constexpr Serial(const Serial& rhs) : data(rhs.data) {};

  // Sets the location of the read/write pointer.
  // If the index is out of range then the pointer is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Bits_8 bin) -> Serial&;
  auto operator<<(const Bits_16 bin) -> Serial&;
  auto operator<<(const Bits_32 bin) -> Serial&;
  auto operator<<(const Bits_64 bin) -> Serial&;
  auto operator<<(const SignedBits_8 bin) -> Serial&;
  auto operator<<(const SignedBits_16 bin) -> Serial&;
  auto operator<<(const SignedBits_32 bin) -> Serial&;
  auto operator<<(const SignedBits_64 bin) -> Serial&;
  auto operator<<(const Real_32 bin) -> Serial&;
  auto operator<<(const Real_64 bin) -> Serial&;
  auto operator<<(const Core::View::Bytes blob) -> Serial&;
  auto operator<<(const Core::View::Vector<Bits_8> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<Bits_16> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<Bits_32> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<Bits_64> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<SignedBits_8> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<SignedBits_16> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<SignedBits_32> blob) -> Serial&;
  auto operator<<(const Core::View::Vector<SignedBits_64> blob) -> Serial&;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr operator View::Bytes() const {
    return View::Bytes(data.get_data(), ptr_location);
  }

 private:
  Core::Access::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Writer
