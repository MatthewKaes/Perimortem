// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/model/types/named_vector.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/model/types/boolean.hpp"
#include "ttx/model/types/real_32.hpp"
#include "ttx/model/types/real_64.hpp"
#include "ttx/model/types/unsigned_32.hpp"
#include "ttx/model/types/unsigned_64.hpp"
#include "ttx/model/types/unsigned_8.hpp"
#include "ttx/model/types/void.hpp"

namespace Tetrodotoxin::Interpreter {

// Builtins owns the fixed common semantic identities a concrete Dialect may
// elect to expose. It is immutable after construction and is not a registry:
// each Dialect explicitly chooses this context and still owns legality for
// every use. Environment bindings never add to or modify this surface.
class Builtins final : public Ttx::Concept::Abstract {
 public:
  using ContractOwner = Builtins;
  static constexpr Perimortem::System::Uuid contract_id{
    0x52b7dc2ef97c4d10,
    0x839d77c613aac2e7,
  };

  Builtins();

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Concept::Abstract::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Builtins"_view;
  }
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_unsigned_64() const
      -> const Ttx::Model::Types::Unsigned_64& {
    return unsigned_64;
  }

 private:
  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Model::Types::Void void_type;
  Ttx::Model::Types::Boolean boolean;
  Ttx::Model::Types::Unsigned_8 unsigned_8;
  Ttx::Model::Types::Unsigned_32 unsigned_32;
  Ttx::Model::Types::Unsigned_64 unsigned_64;
  Ttx::Model::Types::Real_32 real_32;
  Ttx::Model::Types::Real_64 real_64;
  const Model::Types::NamedVector& vec2d;
  const Model::Types::NamedVector& vec4d;
  const Model::Types::NamedVector& uvec2;
};

}  // namespace Tetrodotoxin::Interpreter
