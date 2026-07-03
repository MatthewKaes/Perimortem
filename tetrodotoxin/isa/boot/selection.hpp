// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/registry.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Selection is the Boot instruction payload for `dialect : Name;`.
// The source keyword is still `dialect`, but the value names the ISA that
// evaluates the source body.
class Selection {
 public:
  Selection() = default;
  Selection(Perimortem::Core::View::Bytes name) : name(name) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Registry& registry) -> Selection;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto is_valid() const -> Bool { return !name.is_empty(); }

  constexpr auto operator==(const Selection& rhs) const -> Bool {
    return name == rhs.name;
  }

 private:
  Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Isa
