// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.h"
#include "perimortem/core/option.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// Children2D exposes only the ordered child Objects of one native Object. A
// child retains the configured Type index selected by the compiled Scene.
class Children2D {
 public:
  class Child {
   public:
    constexpr Child() = default;
    constexpr Child(U8* object, Count type_index)
        : object(object), type_index(type_index) {}

    constexpr auto get_object() const -> U8* { return object; }
    constexpr auto get_type_index() const -> Count { return type_index; }

   private:
    U8* object = nullptr;
    Count type_index = Count(-1);
  };

  using ReadCount = Count (*)(const U8*, U8*);
  using Read = Count (*)(const U8*, U8*, Count, U8**);

  constexpr Children2D(const U8* context, ReadCount read_count, Read read)
      : context(context), read_count(read_count), read(read) {}

  auto child_count(U8* object) const -> Count;
  auto child(U8* object, Count index) const -> Perimortem::Core::Option<Child>;

 private:
  const U8* context;
  ReadCount read_count;
  Read read;
};

}  // namespace Tetrodotoxin::Graphics::Runtime
