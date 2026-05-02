// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Serialization::Base64 {

auto decode(Memory::Allocator::Arena& arena, const Core::View::Bytes source)
    -> Core::View::Bytes;

}  // namespace Perimortem::Serialization::Base64
