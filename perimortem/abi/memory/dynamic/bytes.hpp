// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Abi::Memory::Dynamic {

// Bytes is the native carrier for one worker-owned Bibliotheca byte allocation.
// The data pointer owns one reservation while size and capacity remain local to
// this value. Copies share the allocation until a writable owner detaches it.
//
// The carrier cannot move between workers because Bibliotheca reservations are
// thread affine. A borrowed View may cross a worker boundary while the backing
// allocation remains alive, but that View supplies bounds rather than lifetime.
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

  static auto retain(Unsigned_8* data) -> void;
  static auto release(Unsigned_8* data) -> void;
  static auto is_unique(Unsigned_8* data) -> Bool;

  constexpr auto get_data() const -> Unsigned_8* { return data; }
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; }

  constexpr auto set_size(Count selected) -> void { size = selected; }

  constexpr auto replace(
      Unsigned_8* selected_data,
      Count selected_size,
      Count selected_capacity) -> void {
    data = selected_data;
    size = selected_size;
    capacity = selected_capacity;
  }

 private:
  Unsigned_8* data;
  Count size;
  Count capacity;
};

static_assert(sizeof(Bytes) == sizeof(Unsigned_8*) + sizeof(Count) * 2);
static_assert(alignof(Bytes) == alignof(Count));

}  // namespace Perimortem::Abi::Memory::Dynamic

extern "C" auto perimortem_dynamic_bytes_retain(Unsigned_8* data) -> void;
extern "C" auto perimortem_dynamic_bytes_release(Unsigned_8* data) -> void;
