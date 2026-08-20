// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/abi/memory/dynamic/bytes.hpp"

namespace Perimortem::Abi::System::Terminal {

inline constexpr Core::View::Bytes read_line_symbol =
    "perimortem_system_terminal_read_line"_view;
inline constexpr Core::View::Bytes write_line_symbol =
    "perimortem_system_terminal_write_line"_view;

static_assert(
    sizeof(Core::Option<Memory::Dynamic::Bytes>) ==
    sizeof(Memory::Dynamic::Bytes) + sizeof(Count));
static_assert(alignof(Core::Option<Memory::Dynamic::Bytes>) == alignof(Count));

}  // namespace Perimortem::Abi::System::Terminal

// These functions expose the ordinary Perimortem Terminal through the exact
// value carriers used by TTX. Returned Bytes ownership transfers to the caller,
// while the write argument remains borrowed for the duration of the call.
extern "C" auto perimortem_system_terminal_read_line(
    Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>* output)
    -> void;

extern "C" auto perimortem_system_terminal_write_line(
    Perimortem::Abi::Memory::Dynamic::Bytes data) -> bool;
