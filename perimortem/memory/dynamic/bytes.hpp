// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/math.hpp"
#include "perimortem/memory/view/bytes.hpp"

namespace Perimortem::Memory::Dynamic {

// A vector of dynamically managed bytes with value semantics.
class Bytes {
 public:
  Bytes();
  Bytes(Count reserved_capacity);
  Bytes(View::Bytes view);
  Bytes(const Bytes& rhs);
  Bytes(Bytes&& rhs);

  ~Bytes();

  auto append(Byte b) -> void;
  auto concat(View::Bytes view) -> void;
  auto proxy(View::Bytes view) -> void;
  auto resize(Count new_size) -> void;

  auto operator[](Count index) const -> Byte;
  auto at(Count index) const -> Byte;
  auto convert(Byte source, Byte target) -> void;

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(Allocator::Bibliotheca::preface_to_corpus(rented_block),
                       size);
  }

  inline constexpr auto slice(Count start, Count size) const -> View::Bytes {
    if (start >= get_size())
      return View::Bytes();

    return View::Bytes(View::Bytes(
        Allocator::Bibliotheca::preface_to_corpus(rented_block) + start,
        Core::Math::min(size, get_size() - start)));
  };

  constexpr operator View::Bytes() const { return get_view(); }

  auto clear() -> void;
  auto reset() -> void;
  auto get_capacity() const -> Count;
  auto ensure_capacity(Count required_size) -> void;

 private:
  Count size = 0;
  Allocator::Bibliotheca::Preface* rented_block = nullptr;
};

}  // namespace Perimortem::Memory::Dynamic
