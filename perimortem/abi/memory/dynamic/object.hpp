// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Abi::Memory::Dynamic {

// Object defines the native managed Object ABI shared by generated code and
// Perimortem runtime owners. The handle remains the payload address while
// Bibliotheca retains allocation metadata outside the public carrier.
class Object {
 public:
  using Finalizer = void (*)(Unsigned_8*);
  using Cleanup = void (*)();

  static constexpr Perimortem::Core::View::Bytes allocate_symbol =
      "perimortem_dynamic_object_allocate"_view;
  static constexpr Perimortem::Core::View::Bytes retain_symbol =
      "perimortem_dynamic_object_retain"_view;
  static constexpr Perimortem::Core::View::Bytes release_symbol =
      "perimortem_dynamic_object_release"_view;
  static constexpr Perimortem::Core::View::Bytes register_cleanup_symbol =
      "perimortem_dynamic_object_register_cleanup"_view;

  static auto allocate(Count bytes, Finalizer finalizer) -> Unsigned_8*;
  static auto retain(Unsigned_8* payload) -> void;
  static auto release(Unsigned_8* payload) -> void;
  static auto register_cleanup(Cleanup cleanup) -> void;
};

}  // namespace Perimortem::Abi::Memory::Dynamic

extern "C" auto perimortem_dynamic_object_allocate(
    Count bytes,
    Perimortem::Abi::Memory::Dynamic::Object::Finalizer finalizer)
    -> Unsigned_8*;
extern "C" auto perimortem_dynamic_object_retain(Unsigned_8* payload) -> void;
extern "C" auto perimortem_dynamic_object_release(Unsigned_8* payload) -> void;
extern "C" auto perimortem_dynamic_object_register_cleanup(
    Perimortem::Abi::Memory::Dynamic::Object::Cleanup cleanup) -> void;
