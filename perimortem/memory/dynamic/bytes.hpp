// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/amorphous.hpp"
#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/math/hash.hpp"

namespace Perimortem::Memory::Dynamic {

// A vector of dynamically managed bytes with value semantics.
class Bytes {
 public:
  Bytes();
  Bytes(Count reserved_capacity);
  Bytes(Core::View::Amorphous view);
  Bytes(const Bytes& rhs);
  Bytes(Bytes&& rhs);

  auto operator=(Core::View::Amorphous view) -> Bytes& {
    proxy(view);
    return *this;
  }

  auto operator==(const Bytes& rhs) const -> bool {
    return get_view() == rhs.get_view();
  }

  ~Bytes();

  constexpr operator Core::View::Amorphous() const { return get_view(); }
  constexpr operator Core::Access::Amorphous() { return get_access(); }

  auto append(Byte b) -> void;
  auto concat(Core::View::Amorphous view) -> void;
  auto proxy(Core::View::Amorphous view) -> void;
  auto resize(Count new_size) -> void;

  auto operator[](Count index) const -> Byte;
  auto at(Count index) const -> Byte;
  auto convert(Byte source, Byte target) -> void;
  auto slice(Count start, Count size) const -> Core::View::Amorphous;

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count {
    if (rented_block) {
      return rented_block->get_usable_bytes();
    } else {
      return 0;
    }
  }
  constexpr auto get_view() const -> const Core::View::Amorphous {
    return Core::View::Amorphous(
        Allocator::Bibliotheca::preface_to_corpus(rented_block), size);
  }
  constexpr auto get_access() -> Core::Access::Amorphous {
    return Core::Access::Amorphous(
        Allocator::Bibliotheca::preface_to_corpus(rented_block), size);
  }

  constexpr auto hash() const -> Bits_64 {
    return Math::Hash(get_view()).get_value();
  }

  auto clear() -> void;
  auto reset() -> void;
  auto ensure_capacity(Count required_size) -> void;

 private:
  Count size = 0;
  Allocator::Bibliotheca::Preface* rented_block = nullptr;
};

}  // namespace Perimortem::Memory::Dynamic
