// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/hash.hpp"

#include "perimortem/abi/memory/dynamic/bytes.hpp"

namespace Perimortem::Memory::Dynamic {

// A copy-on-write byte value backed by one worker-local Bibliotheca allocation.
// Copies share their allocation until a writable operation detaches one owner.
class Bytes {
 public:
  constexpr Bytes() {};
  Bytes(Count reserved_capacity);
  Bytes(Core::View::Bytes view);
  Bytes(const Dynamic::Bytes& rhs);
  Bytes(Dynamic::Bytes&& rhs);

  auto operator=(Core::View::Bytes view) -> Dynamic::Bytes&;
  auto operator=(const Dynamic::Bytes& rhs) -> Dynamic::Bytes&;
  auto operator=(Dynamic::Bytes&& rhs) -> Dynamic::Bytes&;

  auto operator==(const Dynamic::Bytes& rhs) const -> Bool {
    return get_view() == rhs.get_view();
  }

  auto operator==(const Core::View::Bytes& rhs) const -> Bool {
    return get_view() == rhs;
  }

  ~Bytes();

  constexpr operator Core::View::Bytes() const { return get_view(); }
  constexpr operator Core::Access::Bytes() { return get_access(); }

  auto append(Unsigned_8 byte) -> void;
  auto append(Unsigned_8 byte, Count amount) -> void;
  auto concat(Core::View::Bytes view) -> void;
  auto proxy(Core::View::Bytes view) -> void;
  // Resizes the container but attempts to preserve as much of the original
  // buffer as will fit in the new size.
  //
  // Shrinking the size of the buffer is non-destructive and can be recovered by
  // resetting the size back to it's old value.
  auto resize(Count new_size) -> void;
  // Ensures there is enough room to store a required size, but declares we
  // don't care about the buffer's existing contents.
  //
  // Both growing and shrinking the buffer can be destructive operations so the
  // contents after a forgetful operation should always be assumed to be in an
  // invalid state.
  auto forgetful_resize(Count required_size) -> void;
  // Shrinks the container from the front by a number of bytes.
  //
  // If the container is shrunk more than it's current size the call is
  // equivilant to a clear.
  auto shrink(Count bytes_to_remove) -> void;

  auto operator[](Count index) const -> Unsigned_8;
  auto at(Count index) const -> Unsigned_8;
  auto set(Unsigned_8 target) -> void;
  auto convert(Unsigned_8 source, Unsigned_8 target) -> void;
  auto slice(Count start, Count size) const -> Core::View::Bytes;

  constexpr auto get_size() const -> Count { return storage.get_size(); }
  constexpr auto get_capacity() const -> Count {
    return storage.get_capacity();
  }
  constexpr auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(storage.get_data(), storage.get_size());
  }

  // Access promises writable storage, so a shared allocation detaches before
  // its address escapes. The returned bounds do not extend the Bytes lifetime.
  auto get_access() -> Core::Access::Bytes;

  constexpr auto hash() const -> Unsigned_64 {
    return Core::Hash(get_view()).get_value();
  }

  constexpr auto is_empty() const -> Bool { return storage.get_size() == 0; }

  auto clear() -> void;
  auto reset() -> void;
  auto ensure_capacity(Count required_size) -> void;

 private:
  auto prepare_write(Count required_capacity) -> void;
  auto release() -> void;

  Perimortem::Abi::Memory::Dynamic::Bytes storage;
};

static_assert(sizeof(Bytes) == sizeof(Perimortem::Abi::Memory::Dynamic::Bytes));

}  // namespace Perimortem::Memory::Dynamic
