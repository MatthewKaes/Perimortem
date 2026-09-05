// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Terminal::Abi::Representation {

// Name gives a native producer one readable spelling for a generated
// representation. The semantic path and ABI role stay shared even when
// individual producers choose different native spellings.
class Name {
 public:
  enum class Kind : U8 {
    ImplementationType,
    OptionType,
    ResultType,
    StructureType,
    ObjectType,
    ObjectFinalizer,
    ObjectDescriptor,
  };

  Name(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& semantic,
      Kind kind);

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Terminal::Abi::Representation
