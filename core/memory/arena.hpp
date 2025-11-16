// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/bibliotheca.hpp"

namespace Perimortem::Memory {

// Used for only allocating memory which all shares the same lifetime enabling
// fast allocation / deallocation.
//
// While constructors are called deconstructors are never called for objects
// allocated out of a arena as it's assume they all get wiped together.
// go.
class Arena {
 public:
  // Attempt to request blocks in 16k pages including the preface.
  static constexpr uint64_t page_size =
      (1 << 14) - sizeof(Bibliotheca::Preface);

  Arena();
  ~Arena();

  auto allocate(uint16_t bytes_requested,
                uint8_t alignment = alignof(max_align_t)) -> uint8_t*;
  auto reset() -> void;

  template <typename T>
  inline auto construct(uint32_t count = 1) -> T* {
    T* root = reinterpret_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    for (int i = 0; i < count; i++) {
      new (root + i) T();
    }

    return root;
  }

 private:
  Bibliotheca::Preface* rented_block;
};

}  // namespace Perimortem::Memory
