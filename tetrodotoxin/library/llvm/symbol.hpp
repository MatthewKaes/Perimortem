// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/llvm/unit.hpp"
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
    Construction,
    Address,
    OptionType,
    ResultType,
    StructureType,
    ObjectType,
    ObjectFinalizer,
    ObjectDescriptor,
  };

  Symbol(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& semantic,
      Kind kind,
      Unit unit = {});

  static auto validate(Perimortem::Core::View::Bytes value) -> Bool;

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Library::Llvm
