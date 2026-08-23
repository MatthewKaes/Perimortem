// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "backend/llvm/abi/unit.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Backend::Llvm::Abi {

// Symbol owns one readable native spelling derived from semantic identity.
// Filesystem locations never enter the path, so moving source keeps linkage
// stable.
class Symbol {
 public:
  enum class Kind : U8 {
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

}  // namespace Tetrodotoxin::Backend::Llvm::Abi
