// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/standard_types.hpp"
#include "core/memory/view/bytes.hpp"

namespace Perimortem::Memory::Operations {

void copy(Memory::View::Bytes dest, Memory::View::Bytes src);

}