// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

namespace Perimortem::System {

class Args {
 public:
  struct Config {
    Core::View::Bytes help;
    Bool required = False;
  };

  using Values = Memory::Managed::
      Map<Core::View::Bytes, Memory::Managed::Vector<Core::View::Bytes>*>;

  static auto parse(
      Memory::Allocator::Arena& arena,
      Core::View::Bytes summary,
      Memory::Managed::Map<Core::View::Bytes, Config> variables,
      Core::View::Vector<Core::View::Bytes> arguments) -> Values;
};

}  // namespace Perimortem::System
