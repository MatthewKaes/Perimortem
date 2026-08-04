// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "ttx/model/type.hpp"
#include "ttx/model/types/flag.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

namespace Tetrodotoxin::Library {

// Dialect owns Library interpretation and exposes the immutable binary wide
// intrinsic vocabulary shared by every Monograph it interprets.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  constexpr Dialect(Ttx::Concept::Abstract& registry)
      : Tetrodotoxin::Language::Dialect(registry) {}

  auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& interpretation_context)
      -> Perimortem::Utility::Option<Monograph&> override;

  auto resolve_intrinsic(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;

  // Library scalar identities are binary wide rather than installed Dialect
  // state. Typed access keeps semantic checks out of the authored name path.
  static auto get_bool() -> const Ttx::Model::Types::Flag&;
  static auto get_unsigned_8() -> const Ttx::Model::Types::Unsigned&;
  static auto get_unsigned_16() -> const Ttx::Model::Types::Unsigned&;
  static auto get_unsigned_32() -> const Ttx::Model::Types::Unsigned&;
  static auto get_unsigned_64() -> const Ttx::Model::Types::Unsigned&;
  static auto get_signed_8() -> const Ttx::Model::Types::Signed&;
  static auto get_signed_16() -> const Ttx::Model::Types::Signed&;
  static auto get_signed_32() -> const Ttx::Model::Types::Signed&;
  static auto get_signed_64() -> const Ttx::Model::Types::Signed&;
  static auto get_real_32() -> const Ttx::Model::Types::Real&;
  static auto get_real_64() -> const Ttx::Model::Types::Real&;
  static auto get_void() -> const Ttx::Model::Type&;
};

}  // namespace Tetrodotoxin::Library
