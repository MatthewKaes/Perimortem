// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Storage::Serialization::Textual {

class Stream {
 public:
  Stream(Memory::Dynamic::Bytes& source) : data(source) {};
  Stream(const Stream& rhs) : data(rhs.data) {};

  constexpr operator Memory::View::Bytes() const { return data.get_view(); }

  // Sets the location of the read/write pointer.
  // If the index is out of range then the pointer is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Bool boolean) -> Stream&;
  auto operator<<(const Half half) -> Stream&;
  auto operator<<(const UHalf unsigned_half) -> Stream&;
  auto operator<<(const Int integer) -> Stream&;
  auto operator<<(const UInt unsigned_integer) -> Stream&;
  auto operator<<(const Long full) -> Stream&;
  auto operator<<(const ULong unsigned_full) -> Stream&;
  auto operator<<(const Real_32 real_32) -> Stream&;
  auto operator<<(const Real_64 Real_64) -> Stream&;
  auto operator<<(const Memory::View::Bytes raw) -> Stream&;

  constexpr auto operator[](Count index) const -> Byte { return data[index]; }
  constexpr auto at(Count index) const -> Byte { return data[index]; }

  constexpr auto get_view() const -> const Memory::View::Bytes { return data; }
  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }

 private:
  Memory::Dynamic::Bytes& data;
  Count ptr_location = 0;
};

}  // namespace Perimortem::Storage::Serialization::Textual
