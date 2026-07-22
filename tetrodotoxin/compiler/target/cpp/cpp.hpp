// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/model/library/source.hpp"

namespace Tetrodotoxin::Compiler::Target {

// Emits the host-facing projection of a compiled TTX public interface.
//
// Stable C-linkage declarations name the actual linker exports and use plain
// ABI carriers for every parameter and result. Inline C++ functions marshal
// those carriers into the authored package, type, and dispatch surfaces. This
// keeps classes such as Bool and View::Bytes away from the C-linkage boundary
// without hiding an Addressable function's explicit `self` argument.
//
// Public wrappers use authored TTX type names directly. The generated header
// supplies the small C++ compatibility surface, such as aliasing Void to void,
// without storing target-language spellings on every TTX Type.
class Cpp {
 public:
  static auto build(const Model::Library::Source& program)
      -> Perimortem::Memory::Dynamic::Bytes;
};

}  // namespace Tetrodotoxin::Compiler::Target
