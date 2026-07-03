// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Writer {

// Writes typed values into a dense byte buffer.
//
// Each write emits exactly the bytes for the provided value at the current
// cursor. The writer never inserts alignment padding, so its output can be used
// directly for packed protocol data and worker handoff payloads.
//
// On overflow the writer enters an invalid state and subsequent writes are safe
// but treated as undefined behavior.
template <Data::ByteOrder stream_endian>
class Binary {
 public:
  constexpr Binary(Core::Access::Bytes source) : source(source) {};
  constexpr Binary(const Binary& rhs) : source(rhs.source) {};

  // Sets the location of the read/write cursor.
  // If the index is out of range then the cursor is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Bits_8 value) -> Binary&;
  auto operator<<(const Bits_16 value) -> Binary&;
  auto operator<<(const Bits_32 value) -> Binary&;
  auto operator<<(const Bits_64 value) -> Binary&;
  auto operator<<(const Signed_8 value) -> Binary&;
  auto operator<<(const Signed_16 value) -> Binary&;
  auto operator<<(const Signed_32 value) -> Binary&;
  auto operator<<(const Signed_64 value) -> Binary&;
  auto operator<<(const Real_32 value) -> Binary&;
  auto operator<<(const Real_64 value) -> Binary&;
  auto operator<<(const View::Bytes blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_8> blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_16> blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_32> blob) -> Binary&;
  auto operator<<(const View::Vector<Bits_64> blob) -> Binary&;
  auto operator<<(const View::Vector<Signed_8> blob) -> Binary&;
  auto operator<<(const View::Vector<Signed_16> blob) -> Binary&;
  auto operator<<(const View::Vector<Signed_32> blob) -> Binary&;
  auto operator<<(const View::Vector<Signed_64> blob) -> Binary&;

  constexpr auto get_size() const -> Count { return source.get_size(); }
  constexpr auto get_location() const -> Count { return cursor; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr operator View::Bytes() const {
    return View::Bytes(source.get_data(), cursor);
  }

 private:
  Access::Bytes source;
  Count cursor = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Writer
