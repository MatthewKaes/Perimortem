// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/isa/registry.hpp"
#include "tetrodotoxin/puffer/isa/boot/envelope.hpp"
#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "tetrodotoxin/puffer/resolution/context.hpp"
#include "tetrodotoxin/puffer/resolution/package/cache.hpp"
#include "tetrodotoxin/puffer/resolution/source/cache.hpp"
#include "tetrodotoxin/puffer/resolution/source/roots.hpp"
#include "tetrodotoxin/puffer/resolution/source/snapshot.hpp"

namespace Ttx::Lexical {

class Cursor;
}

namespace Tetrodotoxin::Puffer::Resolution {

// Resolves TTX source requests into cached source records.
//
// Resolver is the source resolution state machine for one package workspace in
// a caller-provided body ISA registry. File imports stay in that source
// workspace. Package imports restore registered Puffer Buffers, so the resolver
// never guesses how a package was built from source.
//
// Only valid resolved sources are cached. If loading or updating a source would
// leave an evaluation error, missing import, ISA mismatch, package failure, or
// cycle in the graph, that source is rejected from the cache and any cached
// consumers made stale by the failed update are removed.
class Resolver {
 public:
  using Context = Resolution::Context;

  explicit Resolver(const Tetrodotoxin::Isa::Registry& isa_registry)
      : isa_registry(isa_registry) {}

  constexpr auto set_package_name(Perimortem::Core::View::Bytes name) -> void {
    package_name = name;
  }

  auto load_source(Context& context, Perimortem::Core::View::Bytes source_path)
      -> Source::Record*;
  auto load_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes ttx_content) -> Source::Record*;
  auto register_package_buffer(
      Context& context,
      Perimortem::Core::View::Bytes buffer_path,
      Perimortem::Core::View::Bytes content) -> Bool;
  auto resolve(Perimortem::Core::View::Bytes import_name) -> Source::Record*;
  auto find_package(Perimortem::Core::View::Bytes package_name) const
      -> const Tetrodotoxin::Archiver::Package*;

  template <typename visitor_type>
  auto visit_reachable(Source::Record& root, visitor_type visit) const -> void {
    visit(root);
    sources.visit_producers(root, visit);
  }

  auto reset() -> void;

 private:
  static auto read_embedded(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes& content) -> Bool;
  auto load_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes ttx_content,
      Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>&
          active_includes,
      const Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>*
          blocked_sources = nullptr) -> Source::Record*;
  auto load_import(
      Context& context,
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes owner_source_path,
      const Tetrodotoxin::Puffer::Isa::Boot::Import& import,
      Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>&
          active_includes,
      const Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>*
          blocked_sources) -> Source::Record*;
  auto evaluate_boot(Ttx::Lexical::Cursor& cursor)
      -> Tetrodotoxin::Puffer::Isa::Boot::Envelope*;
  auto execute_body(
      Ttx::Lexical::Cursor& cursor,
      Source::Record& record,
      const Tetrodotoxin::Isa::Dialect& dialect,
      const Tetrodotoxin::Puffer::Isa::Boot::Envelope& boot,
      Perimortem::Core::View::Vector<Source::Record*> producers) -> Ttx::Type*;
  auto resolve_imports(
      Context& context,
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes owner_source_path,
      const Tetrodotoxin::Puffer::Isa::Boot::Envelope& boot,
      Perimortem::Memory::Dynamic::Vector<Source::Record*>& producers,
      Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>&
          active_includes,
      const Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>*
          blocked_sources) -> Bool;
  auto update_consumers(
      Context& context,
      Perimortem::Memory::Dynamic::Vector<Source::Snapshot>& snapshots) -> void;
  auto snapshot_consumers(
      Source::Record& record,
      Perimortem::Memory::Dynamic::Vector<Source::Snapshot>& snapshots) -> void;

  Source::Cache sources;
  Source::Roots source_roots;
  Package::Cache packages;
  Perimortem::Core::View::Bytes package_name;
  const Tetrodotoxin::Isa::Registry& isa_registry;
};

}  // namespace Tetrodotoxin::Puffer::Resolution
