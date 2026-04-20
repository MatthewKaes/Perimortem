// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/static/bytes.hpp"
#include "perimortem/memory/static/vector.hpp"

namespace Perimortem::System {

class Uuid {
  constexpr Uuid() = default;
  constexpr Uuid(Memory::Static::Vector<Bits_64, 2> source) : value(source) {}
  constexpr Uuid(Memory::Static::Bytes<36> source) {
    // TODO: if consteval {}
    deserialize(source);
  }

  constexpr auto operator==(const Uuid& rhs) const -> Bool {
    return value[0] == rhs.value[0] && value[1] == rhs.value[1];
  }

  constexpr auto get_value() -> Memory::Static::Vector<Bits_64, 2> {
    return value;
  }

  constexpr auto is_valid() -> Bool { return value[0] != 0 && value[1] != 0; }

  auto deserialize(const Memory::Static::Bytes<36>& uuid_string) -> Uuid&;
  auto serialize() const -> Memory::Static::Bytes<36>;

  static auto generate() -> Uuid;

 private:
  Memory::Static::Vector<Bits_64, 2> value = {};
};

}  // namespace Perimortem::System