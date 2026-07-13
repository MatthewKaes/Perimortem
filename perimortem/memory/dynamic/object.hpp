// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Memory::Dynamic {

// Perimortems ref-counted mechanism for light weight thread-local shared
// pointers with automatic lifetime management.
//
// Used for dynamic objects that need persisted state that can be retained by
// multiple systems in the same thread but who's lifetimes are dynamic so they
// can't use the typical Arena lifetime synthesis mechanisms.
//
// Construction always builds a valid object in Bibliotheca storage. Copying or
// move construction reserves that object, move assignment swaps two valid
// handles, and destruction releases one reservation.
template <typename value_type>
class Object {
 public:
  template <typename... arg_types>
  explicit Object(arg_types&&... args) {
    auto allocation = Core::Bibliotheca::check_out(sizeof(value_type));
    value = new (allocation.ptr) value_type(static_cast<arg_types&&>(args)...);
  }

  Object(Object& rhs) : value(rhs.value) {
    Core::Bibliotheca::reserve(Core::Data::cast<Bits_8>(value));
  }

  Object(const Object& rhs) : value(rhs.value) {
    Core::Bibliotheca::reserve(Core::Data::cast<Bits_8>(value));
  }

  Object(Object&& rhs) : Object(rhs) {}

  ~Object() {
    Bits_8* data = Core::Data::cast<Bits_8>(value);
    if (Core::Bibliotheca::reservation_count(data) == 1) {
      value->~value_type();
    }

    Core::Bibliotheca::remit(data);
  }

  auto operator=(const Object& rhs) -> Object& {
    Object copy(rhs);
    Core::Data::swap(value, copy.value);
    return *this;
  }

  auto operator=(Object&& rhs) -> Object& {
    if (this == &rhs) {
      return *this;
    }

    Core::Data::swap(value, rhs.value);
    return *this;
  }

  constexpr auto operator->() -> value_type* { return value; }
  constexpr auto operator->() const -> const value_type* { return value; }
  constexpr auto operator*() -> value_type& { return *value; }
  constexpr auto operator*() const -> const value_type& { return *value; }

 private:
  value_type* value;
};

}  // namespace Perimortem::Memory::Dynamic
