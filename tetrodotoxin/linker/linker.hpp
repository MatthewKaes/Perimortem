// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/object/module.hpp"

namespace Tetrodotoxin::Linker {

// Linker is a stateless native product facade. Every operation consumes one
// complete Module and retains no object or target inventory between products.
class Linker {
 public:
  auto build_library(
      const Object::Module& module,
      Perimortem::Core::View::Bytes object_name) const
      -> Perimortem::Memory::Dynamic::Bytes;
};

}  // namespace Tetrodotoxin::Linker
