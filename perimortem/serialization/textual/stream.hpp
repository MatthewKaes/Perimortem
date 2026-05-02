// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Serialization::Textual {

class Stream {
 public:
  Stream(Core::Access::Bytes source) : data(source) {};
  Stream(const Stream& rhs) : data(rhs.data) {};

  // Sets the location of the read/write pointer.
  // If the index is out of range then the pointer is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Byte character) -> Stream&;
  auto operator<<(const Bool boolean) -> Stream&;
  auto operator<<(const Half half) -> Stream&;
  auto operator<<(const UHalf unsigned_half) -> Stream&;
  auto operator<<(const Int integer) -> Stream&;
  auto operator<<(const UInt unsigned_integer) -> Stream&;
  auto operator<<(const Long full) -> Stream&;
  auto operator<<(const ULong unsigned_full) -> Stream&;
  auto operator<<(const Real_32 real_32) -> Stream&;
  auto operator<<(const Real_64 Real_64) -> Stream&;
  auto operator<<(const Core::View::Bytes raw) -> Stream&;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Count { return valid_state; }

 private:
  Core::Access::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = true;
};

}  // namespace Perimortem::Serialization::Textual
