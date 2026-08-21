// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

namespace Perimortem::Abi::Core {

inline constexpr Perimortem::Core::View::Bytes object_allocate_symbol =
    "perimortem_core_object_allocate"_view;
inline constexpr Perimortem::Core::View::Bytes object_allocate_buffer_symbol =
    "perimortem_core_object_allocate_buffer"_view;
inline constexpr Perimortem::Core::View::Bytes object_retain_symbol =
    "perimortem_core_object_retain"_view;
inline constexpr Perimortem::Core::View::Bytes object_release_symbol =
    "perimortem_core_object_release"_view;
inline constexpr Perimortem::Core::View::Bytes object_capacity_symbol =
    "perimortem_core_object_capacity"_view;
inline constexpr Perimortem::Core::View::Bytes object_clone_symbol =
    "perimortem_core_object_clone"_view;
inline constexpr Perimortem::Core::View::Bytes object_reservations_symbol =
    "perimortem_core_object_reservations"_view;
inline constexpr Perimortem::Core::View::Bytes object_reserve_symbol =
    "perimortem_core_object_reserve"_view;
inline constexpr Perimortem::Core::View::Bytes object_finalize_trivial_symbol =
    "perimortem_core_object_finalize_trivial"_view;
}  // namespace Perimortem::Abi::Core

extern "C" auto perimortem_core_object_allocate(
    const Perimortem::Core::Object<>::Descriptor* descriptor) -> Unsigned_8*;
extern "C" auto perimortem_core_object_allocate_buffer(
    const Perimortem::Core::Object<>::Descriptor* descriptor,
    Count count,
    Count element_size) -> Unsigned_8*;
extern "C" auto perimortem_core_object_retain(Unsigned_8* payload) -> void;
extern "C" auto perimortem_core_object_release(Unsigned_8* payload) -> void;
extern "C" auto perimortem_core_object_capacity(Unsigned_8* payload) -> Count;
extern "C" auto perimortem_core_object_clone(
    Unsigned_8* payload,
    const Perimortem::Core::Object<>::Descriptor* descriptor,
    Count element_size) -> Unsigned_8*;
extern "C" auto perimortem_core_object_reservations(Unsigned_8* payload)
    -> Count;
extern "C" auto perimortem_core_object_reserve(
    Unsigned_8* payload,
    const Perimortem::Core::Object<>::Descriptor* descriptor,
    Count count,
    Count element_size,
    const Unsigned_8* default_value) -> Unsigned_8*;
extern "C" auto perimortem_core_object_finalize_trivial(Unsigned_8*) -> void;
