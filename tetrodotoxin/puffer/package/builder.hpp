// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/terminal.hpp"
#include "tetrodotoxin/puffer/resolution/source/record.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Puffer::Package {

// Builds one durable Puffer Buffer from a resolved package closure.
//
// Every input and diagnostic sink belongs to the calling transaction. Builder
// retains no state after the returned byte view has been written into the
// supplied arena.
class Builder {
 public:
  static auto build(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_name,
      Tetrodotoxin::Puffer::Resolution::Source::Record& root,
      Perimortem::Core::View::Vector<
          Tetrodotoxin::Puffer::Resolution::Source::Record*> records,
      Perimortem::Core::View::Vector<const Tetrodotoxin::Archiver::Package*>
          references,
      Perimortem::Core::View::Vector<Tetrodotoxin::Archiver::Terminal>
          terminals,
      Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Linkage> linkages)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Puffer::Package
