// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Linker::Sections {

// Headers for both ELF and PE binaries are 64 bytes.
constexpr static Count header_size = 64;

struct Header {
  Perimortem::Core::Static::Vector<Byte, header_size> data;
};

}  // namespace Tetrodotoxin::Linker::Sections
