// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Symbol owns one readable native spelling derived from semantic identity.
// Filesystem locations never enter the path, so moving source keeps linkage
// stable.
class Symbol {
 public:
  enum class Kind : Unsigned_8 {
    Path,
    FunctionStatic,
    FunctionSelf,
    Address,
    OptionType,
    StructureType,
    ObjectType,
    ObjectFinalizer,
  };

  Symbol(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& semantic,
      Kind kind);

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Library::Llvm
