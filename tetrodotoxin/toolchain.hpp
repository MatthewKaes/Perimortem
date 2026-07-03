// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/registry.hpp"

namespace Tetrodotoxin {

// Toolchain owns the Tetrodotoxin VM capabilities for one caller.
//
// A CLI, LSP, test harness, embedded host, or package-local resolver can each
// install the ISAs it supports. Full source evaluation still starts by calling
// Boot directly. The registry is the table of body ISAs Boot validates against
// after reading the source envelope.
class Toolchain {
 public:
  Toolchain() = default;

  static auto standard() -> Toolchain;

  auto install_standard_isas() -> void;

  constexpr auto get_isa_registry() -> Isa::Registry& {
    return isa_registry;
  }
  constexpr auto get_isa_registry() const -> const Isa::Registry& {
    return isa_registry;
  }

 private:
  Isa::Registry isa_registry;
};

}  // namespace Tetrodotoxin
