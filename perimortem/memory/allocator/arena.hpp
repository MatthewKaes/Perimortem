// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Memory::Allocator {

// Allocates objects that share one lifetime, allowing fast allocation and
// bulk deallocation.
//
// An arena avoids the bookkeeping overhead of an allocation header for every
// object. This makes it useful when many small objects must be created quickly
// and discarded together, such as a deserialized JSON document or one compiler
// transaction.
//
// `construct()` begins an object's lifetime, but Arena does not call individual
// destructors when it resets. Arena-owned objects must therefore release no
// independently owned resources from their destructors. In particular, rented
// Bibliotheca storage must be remitted before the arena is reset or destroyed.
class Arena {
 public:
  // Attempt to request blocks in 32k pages including the preface and a previous
  // pointer.
  static constexpr Unsigned_64 page_size = (1 << 15);
  static constexpr Unsigned_64 arena_alignment = sizeof(Count);

  Arena();
  Arena(Arena&&);
  ~Arena();
  Arena(const Arena&) = delete;
  auto operator=(const Arena&) -> Arena& = delete;
  auto operator=(Arena&&) -> Arena& = delete;

  inline auto allocate(Count bytes_requested) -> Core::Access::Bytes {
    // Fetch a new page if we are full due to either running out of our current
    // page, or needing to allocate an object larger than our page size.
    //
    // An arena favors cheap allocation over page demotion. A long-lived arena
    // can therefore retain an unusually large page after one large request.
    // Use it for bounded transactions rather than an unbounded object cache.
    if (usage + bytes_requested > page_size) {
      fetch_page(bytes_requested);
    }

    // Align the bump pointer to keep produced data aligned.
    Unsigned_8* root = rented_block + usage;
    usage = Core::Data::align<arena_alignment>(usage + bytes_requested);
    return Core::Access::Bytes(root, bytes_requested);
  }

  // Reserves storage for one object without beginning its lifetime.
  template <typename type>
  auto reserve() -> type& {
    static_assert(alignof(type) <= arena_alignment);
    return *Core::Data::cast<type>(allocate(sizeof(type)).get_data());
  }

  // Reserves storage for a range of object without beginning their lifetime.
  //
  // Useful for allocating a range of POD types that will immediately be
  // assigned.
  template <typename type>
  auto reserve(Count size) -> Core::Access::Vector<type> {
    static_assert(alignof(type) <= arena_alignment);
    auto ptr = allocate(sizeof(type) * size).get_data();
    return Core::Access::Vector<type>(Core::Data::cast<type>(ptr), size);
  }

  // Allocates and constructs one object whose lifetime is owned by the arena.
  template <typename type, typename... arg_types>
  auto construct(arg_types&&... args) -> type& {
    static_assert(alignof(type) <= arena_alignment);
    Unsigned_8* ptr = allocate(sizeof(type)).get_data();
    return *new (ptr) type(static_cast<arg_types&&>(args)...);
  }

  // Creates a duplicate of the target buffer in the current arena.
  // Useful for migrating data from one arena to another.
  auto proxy(Core::View::Bytes source) -> Core::View::Bytes {
    if (source.is_empty()) {
      return Perimortem::Core::View::Bytes();
    }

    auto access = allocate(source.get_size());
    Core::Data::copy(access.get_data(), source.get_data(), source.get_size());
    return access;
  }

  auto reset() -> void;

 private:
  auto fetch_page(Count bytes_requested) -> void;

  Unsigned_8* rented_block;
  Count usage;
};

}  // namespace Perimortem::Memory::Allocator
