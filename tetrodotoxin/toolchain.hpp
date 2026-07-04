// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/registry.hpp"

namespace Tetrodotoxin {

// Toolchain owns the Tetrodotoxin VM capabilities for one caller.
//
// A CLI, LSP, test harness, embedded host, or package-local resolver can each
// install the body ISAs it supports. Source-file preambles are caller-owned;
// Puffer uses its Boot ISA to validate names against this registry before the
// resolver dispatches to a body ISA.
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
