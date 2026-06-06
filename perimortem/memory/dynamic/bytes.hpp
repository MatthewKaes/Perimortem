// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/hash.hpp"

namespace Perimortem::Memory::Dynamic {

// A vector of dynamically managed bytes with value semantics.
class Bytes {
 public:
  Bytes();
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

  auto append(Byte b) -> void;
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

  auto operator[](Count index) const -> Byte;
  auto at(Count index) const -> Byte;
  auto set(Byte target) -> void;
  auto convert(Byte source, Byte target) -> void;
  auto slice(Count start, Count size) const -> Core::View::Bytes;

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; }
  constexpr auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(source_block, size);
  }
  constexpr auto get_access() -> Core::Access::Bytes {
    return Core::Access::Bytes(source_block, size);
  }

  constexpr auto hash() const -> Bits_64 {
    return Core::Hash(get_view()).get_value();
  }

  auto is_empty() { return size == 0; }

  auto clear() -> void;
  auto reset() -> void;
  auto ensure_capacity(Count required_size) -> void;

 private:
  Byte* source_block = nullptr;
  Count size = 0;
  Count capacity = 0;
};

}  // namespace Perimortem::Memory::Dynamic
