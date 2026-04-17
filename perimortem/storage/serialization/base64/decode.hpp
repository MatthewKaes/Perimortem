// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

namespace Perimortem::Storage::Serialization::Base64 {

auto decode(Memory::Allocator::Arena& arena, const Memory::View::Bytes source)
    -> Memory::Managed::Bytes;
}

}  // namespace Perimortem::Storage::Serialization::Base64
