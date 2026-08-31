// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/object.h"

namespace Perimortem::Memory::Dynamic {

// Record adapts the Core Object carrier to C++ object lifetime rules. It is a
// worker local convenience owner for compiler and tooling state, not a language
// Type or another ABI representation.
template <typename value_type>
class Record {
 public:
  template <typename... arg_types>
  Record(arg_types&&... args)
      : object(perimortem_core_object_allocate(&descriptor)) {
    new (object, Core::Placement::Construct)
        value_type(static_cast<arg_types&&>(args)...);
  }

  Record(Record& rhs) : object(rhs.object) {
    perimortem_core_object_retain(object);
  }
  Record(const Record& rhs) : object(rhs.object) {
    perimortem_core_object_retain(object);
  }
  Record(Record&& rhs) : Record(rhs) {}

  auto operator=(const Record& rhs) -> Record& {
    if (object == rhs.object) {
      return *this;
    }

    perimortem_core_object_release(object);
    object = rhs.object;
    perimortem_core_object_retain(object);
    return *this;
  }

  auto operator=(Record&& rhs) -> Record& {
    if (this == &rhs) {
      return *this;
    }

    Core::Data::swap(object, rhs.object);
    return *this;
  }

  ~Record() { perimortem_core_object_release(object); }

  constexpr auto operator->() -> value_type* { return get_value(); }
  constexpr auto operator->() const -> const value_type* { return get_value(); }
  constexpr auto operator*() -> value_type& { return *get_value(); }
  constexpr auto operator*() const -> const value_type& { return *get_value(); }

 private:
  static auto destroy(U8* payload) -> void {
    Core::Data::cast<value_type>(payload)->~value_type();
  }

  constexpr auto get_value() const -> value_type* {
    return Core::Data::cast<value_type>(object);
  }

  inline static constexpr perimortem_object_descriptor descriptor{
    .size = sizeof(value_type),
    .alignment = alignof(value_type),
    .finalize = destroy,
  };

  U8* object = nullptr;
};

static_assert(sizeof(Record<U8>) == sizeof(U8*));

}  // namespace Perimortem::Memory::Dynamic
