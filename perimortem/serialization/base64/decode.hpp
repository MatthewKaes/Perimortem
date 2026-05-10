// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Serialization::Base64 {

auto decode(const Core::View::Bytes source) -> Memory::Dynamic::Bytes;

}  // namespace Perimortem::Serialization::Base64
