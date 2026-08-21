// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Target selects the physical platform contract for one compilation request.
enum class Target : Unsigned_8 {
  X86_64SysV,
};

constexpr auto get_name(Target target) -> Perimortem::Core::View::Bytes {
  switch (target) {
  case Target::X86_64SysV:
    return "x86_64-sysv"_view;
  }
}

}  // namespace Tetrodotoxin::Library::Llvm
