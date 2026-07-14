// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/archiver/dependency.hpp"
#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/terminal.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Archiver {

// Writes a Package into one dense Puffer Buffer.
class Writer {
 public:
  static auto write(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::Core::View::Vector<Dependency> imports,
      const Ttx::Type& root_type,
      Perimortem::Core::View::Vector<const Ttx::Type*> types,
      Perimortem::Core::View::Vector<Terminal> terminals,
      Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Linkage> linkages,
      Perimortem::Core::View::Vector<const Package*> references)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Archiver
