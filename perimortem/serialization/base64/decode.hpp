// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Serialization::Base64 {

auto decode(Memory::Allocator::Arena& arena, const Core::View::Amorphous source)
    -> Core::View::Amorphous;

}  // namespace Perimortem::Serialization::Base64
