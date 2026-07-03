// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Memory::Dynamic {

// A simple linear flat array of trivially constructable values.
template <typename type>
class Vector {
 public:
  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  Vector() {};
  Vector(const Vector& source_vector) {
    ensure_capacity(source_vector.get_size());
    size = source_vector.get_size();

    for (Count i = 0; i < size; i++) {
      new (source_block + i) type(source_vector.source_block[i]);
    }
  }

  Vector(Vector&& source_vector) {
    size = source_vector.size;
    capacity = source_vector.capacity;
    source_block = source_vector.source_block;

    source_vector.size = 0;
    source_vector.capacity = 0;
    source_vector.source_block = nullptr;
  }

  Vector(Count capacity) {
    ensure_capacity(capacity);
    size = 0;
  }

  auto operator=(Vector&& source_vector) -> Vector& {
    size = source_vector.size;
    capacity = source_vector.capacity;

    source_vector.size = 0;
    source_vector.capacity = 0;

    // Swap source blocks. Since move is not destructive, the donor destructor
    // releases the old block this vector used to own.
    Core::Data::swap(source_block, source_vector.source_block);

    return *this;
  }

  auto operator=(const Vector& source_vector) -> Vector& {
    if (this == &source_vector) {
      return *this;
    }

    clear();
    ensure_capacity(source_vector.get_size());
    size = source_vector.get_size();

    for (Count i = 0; i < size; i++) {
      new (source_block + i) type(source_vector.source_block[i]);
    }

    return *this;
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
      Core::Bibliotheca::remit((Bits_8*)source_block);
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

  auto remove(Count index) -> Bool {
    if (index >= size) {
      return False;
    }

    Count last_index = size - 1;
    if (index != last_index) {
      Core::Data::swap(source_block[index], source_block[last_index]);
    }

    source_block[last_index].~type();
    size--;
    return True;
  }

  auto remove_stable(Count index) -> Bool {
    if (index >= size) {
      return False;
    }

    Count last_index = size - 1;
    for (Count shift_index = index; shift_index < last_index; shift_index++) {
      Core::Data::swap(source_block[shift_index], source_block[shift_index + 1]);
    }

    source_block[last_index].~type();
    size--;
    return True;
  }

  // Resizes the container but attempts to preserve as much of the original
  // buffer as will fit in the new size.
  //
  // Shrinking the size of the buffer is non-destructive and can be recovered by
  // resetting the size back to it's old value.
  auto resize(Count new_size) -> void {
    ensure_capacity(new_size);
    size = new_size;
  }

  // Ensures there is enough room to store a required size, but declares we
  // don't care about the buffer's existing contents.
  //
  // Both growing and shrinking the buffer can be destructive operations so the
  // contents after a forgetful operation should always be assumed to be in an
  // invalid state.
  auto forgetful_resize(Count required_size) -> void {
    // Always set the size.
    size = required_size;

    // Get the capacity bounds and check if we need a realloc.
    // If the block fits in the current Bibliotheca archive then reuse it.
    // If the block size requires at least one step up or step down then request
    // a new block.
    if (required_size <= capacity && required_size > (capacity >> 1)) {
      return;
    }

    if (source_block) {
      Core::Bibliotheca::remit((Bits_8*)source_block);
    }

    auto alloc = Core::Bibliotheca::check_out(required_size * sizeof(type));
    source_block = Core::Data::cast<type>(alloc.ptr);
    capacity = alloc.capacity / sizeof(type);
  }

  constexpr auto contains(const type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> const type& {
    return source_block[index];
  }
  constexpr auto at(Count index) -> type& { return source_block[index]; }
  constexpr auto operator[](Count index) const -> const type& {
    return at(index);
  }
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
  auto destruct() -> void {
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
    const auto new_capacity =
        Core::Math::max(get_capacity() * 2, required_size);

    // Fetch and transfer to new block.
    auto alloc = Core::Bibliotheca::check_out(new_capacity * sizeof(type));
    auto new_block = Core::Data::cast<type>(alloc.ptr);

    if (source_block) {
      if constexpr (__is_trivially_copyable(type)) {
        memcpy(new_block, source_block, sizeof(type) * size);
      } else {
        for (Count i = 0; i < size; i++) {
          new (new_block + i) type(source_block[i]);
        }
        destruct();
      }
      Core::Bibliotheca::remit((Bits_8*)source_block);
    }

    // Update block and get the new capacity.
    source_block = new_block;

    // Get the actual capacity provided which is often more than we actual
    // requested.
    capacity = alloc.capacity / sizeof(type);
  }

  type* source_block = nullptr;
  Count size = 0;
  Count capacity = 0;
};

}  // namespace Perimortem::Memory::Dynamic
