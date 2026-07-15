// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/puffer/resolution/context.hpp"

namespace Tetrodotoxin::Puffer {

// Runs a complete Puffer compilation transaction.
//
// Compiler has no persistent state. The caller supplies the diagnostic context
// and receives every derived artifact as one Output value. Library and package
// builds are separate operations because package discovery starts at one
// package.ttx root and produces a durable package buffer, while a standalone
// library lowers each supplied source root directly.
class Compiler {
 public:
  struct Output {
    // The current backend's native static library.
    Perimortem::Memory::Dynamic::Bytes native_archive;
    // The public C++ projection of the TTX interface.
    Perimortem::Memory::Dynamic::Bytes cpp_header;
    // The durable package snapshot. Library builds leave this empty because
    // they do not publish a package identity.
    Perimortem::Memory::Dynamic::Bytes package_buffer;
  };

  static auto build_library(
      Tetrodotoxin::Puffer::Resolution::Context& context,
      Perimortem::Core::View::Bytes unit_name,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          dependencies,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> sources)
      -> Output;
  static auto build_package(
      Tetrodotoxin::Puffer::Resolution::Context& context,
      Perimortem::Core::View::Bytes unit_name,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>
          dependencies,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> sources)
      -> Output;
};

}  // namespace Tetrodotoxin::Puffer
