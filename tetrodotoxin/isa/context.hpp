// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"

#include "tetrodotoxin/standard/types.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa {

// Context is the pushdown state shared by a single ISA evaluation.
//
// It owns the arena used to synthesize durable TTX objects for that source
// transaction and the type names made visible by imports or earlier evaluated
// declarations. It deliberately does not parse type expressions; that grammar
// belongs to Expression::Type.
//
// Temporary scratch data should stay with the cursor or a local container. This
// arena is for objects that must survive as part of the produced TTX model.
class Context {
 public:
  explicit Context(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena) {}

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }
  auto define_type(Perimortem::Core::View::Bytes name, const Ttx::Type& type)
      -> Bool {
    if (types.find(name) != nullptr ||
        Tetrodotoxin::Standard::Types::is_type(name)) {
      return False;
    }

    types.insert(name, &type);
    return True;
  }

  auto define_type(const Ttx::Type& type) -> Bool {
    return define_type(type.get_name(), type);
  }

  auto find_type(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Type* {
    const auto* type = types.find(name);
    return type == nullptr ? nullptr : type->value;
  }

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      const Ttx::Type*>
      types;
};

}  // namespace Tetrodotoxin::Isa
