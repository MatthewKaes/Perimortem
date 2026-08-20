// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core {

// Object is the one word managed reference used by the Core ABI. Its payload
// follows one allocation adjacent control record which points at an immutable
// descriptor. Copy and destruction policy remain explicit ABI operations, so
// the carrier itself stays trivially copyable.
class Object {
 public:
  using Finalizer = void (*)(Unsigned_8*);

  class Descriptor {
   public:
    constexpr Descriptor(Count size, Count alignment, Finalizer finalizer)
        : size(size), alignment(alignment), finalizer(finalizer) {}

    constexpr auto get_size() const -> Count { return size; }
    constexpr auto get_alignment() const -> Count { return alignment; }
    constexpr auto get_finalizer() const -> Finalizer { return finalizer; }

   private:
    Count size;
    Count alignment;
    Finalizer finalizer;
  };

  explicit constexpr Object(Unsigned_8* payload) : payload(payload) {}

  static auto create(const Descriptor& descriptor) -> Object;

  auto retain() const -> void;
  auto release() const -> void;

  constexpr auto get_payload() const -> Unsigned_8* { return payload; }
  auto get_descriptor() const -> const Descriptor&;

 private:
  class Control {
   public:
    constexpr Control(const Descriptor& descriptor) : descriptor(&descriptor) {}

    constexpr auto get_descriptor() const -> const Descriptor& {
      return *descriptor;
    }

   private:
    const Descriptor* descriptor;
  };

  auto get_control() const -> Control&;

  Unsigned_8* payload;
};

static_assert(sizeof(Object) == sizeof(Unsigned_8*));
static_assert(alignof(Object) == alignof(Unsigned_8*));
static_assert(__is_trivially_copyable(Object));
static_assert(
    sizeof(Object::Descriptor) ==
    sizeof(Count) * 2 + sizeof(Object::Finalizer));
static_assert(alignof(Object::Descriptor) == alignof(Count));
static_assert(__is_standard_layout(Object::Descriptor));

}  // namespace Perimortem::Core
