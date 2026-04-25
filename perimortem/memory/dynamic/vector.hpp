// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/structured.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/utility/func/math.hpp"

namespace Perimortem::Memory::Dynamic {

// A simple linear flat array of trivially constructable values.
template <typename value_type, Bool storage_only = false>
class Vector {
 public:
  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  Vector() {};
  Vector(const Vector& rhs) {
    ensure_capacity(rhs.get_size() * sizeof(value_type));
    size = rhs.get_size();

    if constexpr (storage_only) {
      memcpy(access(rented_block), access(rhs.rented_block),
             sizeof(value_type) * size);
    } else {
      auto target_items = access(rented_block);
      auto source_items = access(rhs.rented_block);
      for (Count i = 0; i < size; i++) {
        new (target_items + i) value_type(source_items[i]);
      }
    }
  }

  Vector(Vector&& rhs) {
    size = rhs.size;
    capacity = rhs.capacity;
    rented_block = rhs.rented_block;

    rhs.size = 0;
    rhs.capacity = 0;
    rhs.rented_block = nullptr;
  }

  Vector(Count capacity) {
    ensure_capacity(capacity);
    size = 0;
  }

  ~Vector() { reset(); }

  constexpr operator Core::View::Structured<value_type>() const {
    return Core::View::Structured<value_type>(rented_block, size);
  }

  auto clear() -> void {
    if (rented_block) {
      destruct();
    }
    size = 0;
  }

  // Unlike the clear function, reset returns the actual buffer.
  // Useful if the vector will be reused to store highly variable size objects.
  auto reset() -> void {
    if (rented_block) {
      destruct();
      Allocator::Bibliotheca::remit(rented_block);
    }

    size = 0;
    capacity = 0;
    rented_block = nullptr;
  }

  constexpr auto insert(const value_type& data) -> value_type& {
    ensure_capacity(size + 1);

    // Construct using the copy constructor.
    return *new (rented_block + (size++)) value_type(data);
  }

  constexpr auto emplace(const value_type&& data) -> value_type& {
    ensure_capacity(size + 1);

    // Construct using the move constructor.
    return *new (rented_block + (size++)) value_type(data);
  }

  constexpr auto contains(const value_type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (rented_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> value_type& {
    return rented_block[index];
  }
  constexpr auto operator[](Count index) -> value_type& { return at(index); }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; };
  constexpr auto get_view() const -> Core::View::Structured<value_type> {
    return Core::View::Structured<value_type>(rented_block, size);
  }

 private:
  static constexpr auto access(Allocator::Bibliotheca::Preface* preface)
      -> value_type* {
    return reinterpret_cast<value_type*>(
        Allocator::Bibliotheca::preface_to_corpus(preface));
  }

  auto destruct() -> void {
    // Optimizer will do this anyway, but making it explicit helps catch issues
    // in debug builds.
    if constexpr (storage_only) {
      return;
    }

    // Look over all entries and destruct the keys and values.
    auto buckets = access(rented_block);
    for (Count bucket_index = 0; bucket_index < size; bucket_index++) {
      buckets[bucket_index].~value_type();
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
    const auto new_capacity =
        Core::Math::max(get_capacity() * 2, required_size);

    // Fetch and transfer to new block.
    auto new_block =
        Allocator::Bibliotheca::check_out(new_capacity * sizeof(value_type));

    if (rented_block) {
      memcpy(access(new_block), access(rented_block),
             sizeof(value_type) * size);
      Allocator::Bibliotheca::remit(rented_block);
    }

    // Update block and get the new capacity.
    rented_block = new_block;

    // Get the actual capacity provided which is often more than we actual
    // requested.
    capacity = new_block->get_usable_bytes() / sizeof(value_type);
  }

  Allocator::Bibliotheca::Preface* rented_block = nullptr;
  Count size = 0;
  Count capacity = 0;
};

}  // namespace Perimortem::Memory::Dynamic
