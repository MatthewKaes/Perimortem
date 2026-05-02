// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/view/bytes.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"

namespace Perimortem::Memory::Dynamic {

// A vector of dynamically managed bytes with value semantics.
class Bytes {
 public:
  Bytes();
  Bytes(Count reserved_capacity);
  Bytes(Core::View::Bytes view);
  Bytes(const Bytes& rhs);
  Bytes(Bytes&& rhs);

  auto operator=(Core::View::Bytes view) -> Bytes& {
    proxy(view);
    return *this;
  }

  auto operator=(const Bytes& view) -> Bytes& {
    proxy(view);
    return *this;
  }

  auto operator==(const Bytes& rhs) const -> Bool {
    return get_view() == rhs.get_view();
  }
  auto operator!=(const Bytes& rhs) const -> Bool {
    return get_view() != rhs.get_view();
  }

  ~Bytes();

  constexpr operator Core::View::Bytes() const { return get_view(); }
  constexpr operator Core::Access::Bytes() { return get_access(); }

  auto append(Byte b) -> void;
  auto concat(Core::View::Bytes view) -> void;
  auto proxy(Core::View::Bytes view) -> void;
  auto resize(Count new_size) -> void;

  auto operator[](Count index) const -> Byte;
  auto at(Count index) const -> Byte;
  auto convert(Byte source, Byte target) -> void;
  auto slice(Count start, Count size) const -> Core::View::Bytes;

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count {
    if (source_block) {
      return Allocator::Bibliotheca::corpus_to_preface(source_block)
          ->get_usable_bytes();
    } else {
      return 0;
    }
  }
  constexpr auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(source_block, size);
  }
  constexpr auto get_data() const -> const Byte* { return source_block; }
  constexpr auto get_access() -> Core::Access::Bytes {
    return Core::Access::Bytes(source_block, size);
  }

  constexpr auto hash() const -> Bits_64 {
    return Core::Hash(get_view()).get_value();
  }

  auto clear() -> void;
  auto reset() -> void;
  auto ensure_capacity(Count required_size) -> void;

 private:
  Count size = 0;
  Byte* source_block = nullptr;
};

}  // namespace Perimortem::Memory::Dynamic
