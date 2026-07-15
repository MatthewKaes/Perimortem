// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/object.hpp"
#include "perimortem/memory/dynamic/set.hpp"

#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/isa/dialect.hpp"
#include "tetrodotoxin/isa/registry.hpp"
#include "tetrodotoxin/puffer/resolution/context.hpp"
#include "tetrodotoxin/puffer/resolution/package/buffer.hpp"
#include "tetrodotoxin/puffer/resolution/package/restored.hpp"
#include "tetrodotoxin/puffer/resolution/source/cache.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Package {

// Package import cache for registered Puffer Buffers.
//
// Resolver handles source traversal. Package::Cache handles the compiled
// package side of that graph: manifest registration, dependency version checks,
// delayed package restore, and ownership of the restored Package and graph
// Record that Source::Cache indexes.
class Cache {
 public:
  auto register_buffer(
      Context& context,
      Perimortem::Core::View::Bytes buffer_path,
      Perimortem::Core::View::Bytes content) -> Bool;
  auto load(
      Context& context,
      Ttx::Lexical::Cursor& cursor,
      Source::Cache& sources,
      const Tetrodotoxin::Isa::Dialect& dialect,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>&
          active_includes) -> Source::Record*;
  auto find(Perimortem::Core::View::Bytes package_name) const
      -> const Tetrodotoxin::Archiver::Package*;
  auto reset() -> void {
    restored.clear();
    buffers.clear();
  }

 private:
  auto find_buffer(Perimortem::Core::View::Bytes package_name) -> Buffer*;

  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      Perimortem::Memory::Dynamic::Object<Buffer>>
      buffers;
  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, Restored>
      restored;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Package
