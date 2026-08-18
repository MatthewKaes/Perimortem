// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

#include "perimortem/abi/memory/dynamic/object.hpp"

namespace Perimortem::Memory::Dynamic {

// Object is Perimortem's reference counted handle for lightweight state shared
// within one worker. It suits values whose independent lifetimes cannot use an
// Arena transaction.
//
// Construction always builds a valid object in Bibliotheca storage. Copying or
// move construction reserves that object, move assignment swaps two valid
// handles, and destruction releases one reservation.
template <typename value_type>
class Object {
 public:
  template <typename... arg_types>
  Object(arg_types&&... args) {
    value = new (::Perimortem::Abi::Memory::Dynamic::Object::allocate(
        sizeof(value_type), destroy))
        value_type(static_cast<arg_types&&>(args)...);
  }

  Object(Object& rhs) : value(rhs.value) {
    ::Perimortem::Abi::Memory::Dynamic::Object::retain(
        Core::Data::cast<Unsigned_8>(value));
  }

  Object(const Object& rhs) : value(rhs.value) {
    ::Perimortem::Abi::Memory::Dynamic::Object::retain(
        Core::Data::cast<Unsigned_8>(value));
  }

  Object(Object&& rhs) : Object(rhs) {}

  ~Object() { release(); }

  auto operator=(const Object& rhs) -> Object& {
    if (rhs.value == value) {
      return *this;
    }

    release();
    value = rhs.value;
    ::Perimortem::Abi::Memory::Dynamic::Object::retain(
        Core::Data::cast<Unsigned_8>(value));
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
  static auto destroy(Unsigned_8* payload) -> void {
    Core::Data::cast<value_type>(payload)->~value_type();
  }

  auto release() -> void {
    ::Perimortem::Abi::Memory::Dynamic::Object::release(
        Core::Data::cast<Unsigned_8>(value));
  }

  value_type* value;
};

}  // namespace Perimortem::Memory::Dynamic
