// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

#include "tetrodotoxin/linker/target/elf.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;

auto Linker::build_library(
    const Object::Module& module,
    View::Bytes object_name) const -> Dynamic::Bytes {
  Target::Elf format;
  return format.build_library(module, object_name);
}
