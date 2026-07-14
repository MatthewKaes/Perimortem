// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/type.hpp"

namespace Tetrodotoxin::Abi {

// Describes the complete machine representation selected for a TTX value.
//
// The source spelling is translated once when the attribute is evaluated and
// stored through Attribute's unsigned scalar alternative. Zero is reserved for
// Invalid, so a missing or key-only attribute remains safe to decode. Targets
// consume the scalar and never repeat textual classification or infer signed
// and boolean behavior from type names.
enum class Lowering : Bits_8 {
  Invalid = 0,
  Void = 1,
  Bool = 2,
  Integer = 3,
  Signed = 4,
  Real = 5,
  ViewBytes = 6,
};

// Type is one public TTX identity selected for a host-language interface.
//
// The public path is binding metadata, while the referenced Ttx::Type remains
// the sole owner of layout, documentation, aliases, and target attributes. This
// projection therefore does not manufacture a second type model merely to
// generate a C++ header.
class Type {
 public:
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes public_path,
      const Ttx::Type& type) -> Type;

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return public_path;
  }

  constexpr auto get_type() const -> const Ttx::Type& { return type; }

 private:
  Type(Perimortem::Core::View::Bytes public_path, const Ttx::Type& type)
      : public_path(public_path), type(type) {}

  Perimortem::Core::View::Bytes public_path;
  const Ttx::Type& type;
};

}  // namespace Tetrodotoxin::Abi
