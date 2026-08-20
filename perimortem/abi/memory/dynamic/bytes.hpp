// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Abi::Memory::Dynamic {

// Bytes exposes the native symbols and borrowed physical carrier used by
// generated code. Memory::Dynamic::Bytes remains the owning Perimortem value.
// Retain and release observe the complete carrier selected by authored Type
// Attributes. Concat transfers one newly owned value to its caller. This
// interface adds no second lifetime or storage implementation.
class Bytes {
 public:
  constexpr Bytes(
      Unsigned_8* data = nullptr,
      Count size = 0,
      Count capacity = 0)
      : data(data), size(size), capacity(capacity) {}

  static constexpr Perimortem::Core::View::Bytes retain_symbol =
      "perimortem_dynamic_bytes_retain"_view;
  static constexpr Perimortem::Core::View::Bytes release_symbol =
      "perimortem_dynamic_bytes_release"_view;
  static constexpr Perimortem::Core::View::Bytes concat_symbol =
      "perimortem_dynamic_bytes_concat"_view;

  constexpr auto get_data() const -> Unsigned_8* { return data; }
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; }

 private:
  Unsigned_8* data;
  Count size;
  Count capacity;
};

static_assert(sizeof(Bytes) == sizeof(Unsigned_8*) + sizeof(Count) * 2);
static_assert(alignof(Bytes) == alignof(Count));

}  // namespace Perimortem::Abi::Memory::Dynamic

extern "C" auto perimortem_dynamic_bytes_retain(
    const Perimortem::Abi::Memory::Dynamic::Bytes* value) -> void;
extern "C" auto perimortem_dynamic_bytes_release(
    const Perimortem::Abi::Memory::Dynamic::Bytes* value) -> void;

extern "C" auto perimortem_dynamic_bytes_concat(
    Perimortem::Memory::Dynamic::Bytes* output,
    Perimortem::Core::View::Bytes left,
    Perimortem::Core::View::Bytes right) -> void;
