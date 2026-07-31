// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/target/format.hpp"

namespace Tetrodotoxin::Linker::Target {

// Elf emits the current System V archive smoke around one relocatable object.
// Object grouping and wire descriptors remain local to that encoding call.
class Elf : public Format {
 public:
  auto build_library(
      const Object::Module& module,
      Perimortem::Core::View::Bytes object_name) const
      -> Perimortem::Memory::Dynamic::Bytes override;
};

}  // namespace Tetrodotoxin::Linker::Target
