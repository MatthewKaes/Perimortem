// Perimortem Engine
// Copyright © Matt Kaes

#include "core/memory/operations.hpp"

#include <string.h>

namespace Perimortem::Memory::Operations {

void copy(Memory::View::Bytes dest, Memory::View::Bytes src) {
  Count safe_transfer = dest.get_size() <= src.get_size() ? dest.get_size() : src.get_size();
  std::memcpy(const_cast<char*>(dest.get_data()), src.get_data(), safe_transfer);
}

}