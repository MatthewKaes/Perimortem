// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/math/basic.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"

namespace Perimortem::Memory::Dynamic {

// A simple linear flat array of trivially constructable values.
template <typename type, Bool storage_only = false>
class Vector {
 public:
  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  Vector() {};
  Vector(const Vector& rhs) {
    ensure_capacity(rhs.get_size() * sizeof(type));
    size = rhs.get_size();

    if constexpr (storage_only) {
      memcpy(source_block, rhs.source_block, sizeof(type) * size);
    } else {
      auto target_items = source_block;
      auto source_items = rhs.source_block;
      for (Count i = 0; i < size; i++) {
        new (target_items + i) type(source_items[i]);
      }
    }
  }

  Vector(Vector&& rhs) {
    size = rhs.size;
    capacity = rhs.capacity;
    source_block = rhs.source_block;

    rhs.size = 0;
    rhs.capacity = 0;
    rhs.source_block = nullptr;
  }

  Vector(Count capacity) {
    ensure_capacity(capacity);
    size = 0;
  }

  ~Vector() { reset(); }

  constexpr operator Core::View::Vector<type>() const { return get_view(); }
  constexpr operator Core::Access::Vector<type>() { return get_access(); }

  auto clear() -> void {
    if (source_block) {
      destruct();
    }
    size = 0;
  }

  // Unlike the clear function, reset returns the actual buffer.
  // Useful if the vector will be reused to store highly variable size objects.
  auto reset() -> void {
    if (source_block) {
      destruct();
      Allocator::Bibliotheca::remit(
          Allocator::Bibliotheca::corpus_to_preface((Bits_8*)source_block));
    }

    size = 0;
    capacity = 0;
    source_block = nullptr;
  }

  constexpr auto insert(const type& data) -> type& {
    ensure_capacity(size + 1);

    // Construct using the copy constructor.
    return *new (source_block + (size++)) type(data);
  }

  constexpr auto emplace(const type&& data) -> type& {
    ensure_capacity(size + 1);

    // Construct using the move constructor.
    return *new (source_block + (size++)) type(data);
  }

  constexpr auto contains(const type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> type& { return source_block[index]; }
  constexpr auto operator[](Count index) -> type& { return at(index); }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; }
  constexpr auto get_view() const -> const Core::View::Vector<type> {
    return Core::View::Vector<type>(source_block, get_size());
  }
  constexpr auto get_data() const -> const type* { return source_block; }
  constexpr auto get_data() -> type* { return source_block; }
  constexpr auto get_access() -> Core::Access::Vector<type> {
    return Core::Access::Vector<type>(source_block, get_size());
  }

 private:
  static constexpr auto access(Allocator::Bibliotheca::Preface* preface)
      -> type* {
    return reinterpret_cast<type*>(
        Allocator::Bibliotheca::preface_to_corpus(preface));
  }

  auto destruct() -> void {
    // Optimizer will do this anyway, but making it explicit helps catch issues
    // in debug builds.
    if constexpr (storage_only) {
      return;
    }

    // Look over all entries and destruct the keys and values.
    for (Count bucket_index = 0; bucket_index < size; bucket_index++) {
      source_block[bucket_index].~type();
    }
  }

  // Ensures there is _at least_ enough room for the requested number of
  // objects.
  auto ensure_capacity(Count required_size) -> void {
    // Check if we can already fit required buffer.
    if (required_size <= get_capacity()) {
      return;
    }

    // Attempt to grow by a factor of 2.
    // If that doesn't work than grow to exact size.
    const auto new_capacity = Math::max(get_capacity() * 2, required_size);

    // Fetch and transfer to new block.
    auto preface =
        Allocator::Bibliotheca::check_out(new_capacity * sizeof(type));
    auto new_block = access(preface);

    if (source_block) {
      memcpy(new_block, source_block, sizeof(type) * size);
      Allocator::Bibliotheca::remit(
          Allocator::Bibliotheca::corpus_to_preface((Bits_8*)source_block));
    }

    // Update block and get the new capacity.
    source_block = new_block;

    // Get the actual capacity provided which is often more than we actual
    // requested.
    capacity = preface->get_usable_bytes() / sizeof(type);
  }

  type* source_block = nullptr;
  Count size = 0;
  Count capacity = 0;
};

}  // namespace Perimortem::Memory::Dynamic
