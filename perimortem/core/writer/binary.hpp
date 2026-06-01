// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Writer {

// Reads typed values from a flat byte buffer with alignment padding between
// fields.
//
// Each read advances the pointer to the next natural alignment boundary for
// that type before consuming bytes. Raw-byte reads via read_bytes() bypass
// alignment and always read from the current position allowing reads from any
// arbitrary location, however it does not convert the bytes into Native endian
// and simply returns a View.
//
// On overflow the writer enters an invalid state and subsequent writes are safe
// but treated as undefined behavior.
template <Data::ByteOrder stream_endian>
class Binary {
 public:
  constexpr Binary(Core::Access::Bytes data) : data(data) {};
  constexpr Binary(const Binary& rhs) : data(rhs.data) {};

  // Sets the location of the read/write pointer.
  // If the index is out of range then the pointer is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Bits_8 bin) -> Binary&;
  auto operator<<(const Bits_16 bin) -> Binary&;
  auto operator<<(const Bits_32 bin) -> Binary&;
  auto operator<<(const Bits_64 bin) -> Binary&;
  auto operator<<(const SignedBits_8 bin) -> Binary&;
  auto operator<<(const SignedBits_16 bin) -> Binary&;
  auto operator<<(const SignedBits_32 bin) -> Binary&;
  auto operator<<(const SignedBits_64 bin) -> Binary&;
  auto operator<<(const Real_32 bin) -> Binary&;
  auto operator<<(const Real_64 bin) -> Binary&;
  auto operator<<(const View::Bytes blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_8> blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_16> blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_32> blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_64> blob) -> Binary&;
  auto operator<<(const View::Vector<SignedBits_8> blob) -> Binary&;
  auto operator<<(const View::Vector<SignedBits_16> blob) -> Binary&;
  auto operator<<(const View::Vector<SignedBits_32> blob) -> Binary&;
  auto operator<<(const View::Vector<SignedBits_64> blob) -> Binary&;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr operator View::Bytes() const {
    return View::Bytes(data.get_data(), ptr_location);
  }

 private:
  Access::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Writer
