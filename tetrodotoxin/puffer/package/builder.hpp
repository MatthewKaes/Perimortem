// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/reference.hpp"
#include "tetrodotoxin/archiver/terminal.hpp"
#include "tetrodotoxin/puffer/resolution/source/record.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Puffer::Resolution {

class Resolver;
}

namespace Tetrodotoxin::Puffer::Package {

// Builder turns one resolved package closure into its durable Puffer Buffer.
// Resolution supplies stable records and terminal production supplies byte
// artifacts; Builder owns only dependency closure and archive assembly.
class Builder {
 public:
  Builder(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors)
      : arena(arena), errors(errors) {}

  auto build(
      Tetrodotoxin::Puffer::Resolution::Resolver& resolver,
      Tetrodotoxin::Puffer::Resolution::Source::Record& root,
      Perimortem::Core::View::Vector<
          Tetrodotoxin::Puffer::Resolution::Source::Record*> records,
      Perimortem::Core::View::Vector<Tetrodotoxin::Archiver::Terminal>
          terminals) -> Perimortem::Core::View::Bytes;

 private:
  using Record = Tetrodotoxin::Puffer::Resolution::Source::Record;

  auto report(Record& root, Perimortem::Core::View::Bytes message) -> void;

  static auto package_name(const Ttx::Type& type)
      -> Perimortem::Core::View::Bytes;

  Perimortem::Memory::Allocator::Arena& arena;
  Ttx::Lexical::Errors& errors;
  Perimortem::Memory::Dynamic::
      Map<Perimortem::Core::View::Bytes, Tetrodotoxin::Archiver::Reference>
          packages;
};

}  // namespace Tetrodotoxin::Puffer::Package
