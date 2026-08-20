// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

namespace Perimortem::Abi::Core {

inline constexpr Perimortem::Core::View::Bytes object_allocate_symbol =
    "perimortem_core_object_allocate"_view;
inline constexpr Perimortem::Core::View::Bytes object_retain_symbol =
    "perimortem_core_object_retain"_view;
inline constexpr Perimortem::Core::View::Bytes object_release_symbol =
    "perimortem_core_object_release"_view;
}  // namespace Perimortem::Abi::Core

extern "C" auto perimortem_core_object_allocate(
    const Perimortem::Core::Object::Descriptor* descriptor) -> Unsigned_8*;
extern "C" auto perimortem_core_object_retain(Unsigned_8* payload) -> void;
extern "C" auto perimortem_core_object_release(Unsigned_8* payload) -> void;
