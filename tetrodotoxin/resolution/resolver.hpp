// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/resolution/source/cache.hpp"
#include "ttx/lexical/error.hpp"

namespace Tetrodotoxin::Resolution {

// Resolves TTX source requests into cached source envelopes.
//
// Resolver is the source-tree state machine for one package boundary. A naked
// Resolver is the default root package used by tools such as the CLI and LSP.
// Packages can be represented by their package source and may later grow their
// own local resolver for private package files; public lookup still goes
// through package names rather than private source paths.
//
// Only valid resolved sources are cached. If loading or updating a source would
// leave a parse error, missing import, dialect mismatch, package failure, or
// cycle in the graph, that source is rejected from the cache and any cached
// consumers made stale by the failed update are removed.
class Resolver {
 public:
  class Context {
   public:
    auto reset() -> void;
    auto add_error(const Ttx::Lexical::Error& error) -> void;

    constexpr auto has_errors() const -> Bool {
      return errors.get_size() != 0;
    }
    constexpr auto get_errors() const
        -> Perimortem::Core::View::Vector<Ttx::Lexical::Error> {
      return errors.get_view();
    }

   private:
    auto keep(Perimortem::Core::View::Bytes bytes)
        -> Perimortem::Core::View::Bytes;

    Perimortem::Memory::Allocator::Arena error_arena;
    Perimortem::Memory::Dynamic::Vector<Ttx::Lexical::Error> errors;
  };

  auto load_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path) -> Source::Record*;
  auto load_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes ttx_content) -> Source::Record*;
  auto resolve(Perimortem::Core::View::Bytes import_name) -> Source::Record*;
  auto reset() -> void;

 private:
  struct Snapshot {
    Perimortem::Memory::Dynamic::Bytes source_path;
    Perimortem::Memory::Dynamic::Bytes source_text;
  };

  auto load_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes ttx_content,
      Bool private_source,
      Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes>&
          resolving,
      const Perimortem::Memory::Dynamic::Vector<
          Perimortem::Memory::Dynamic::Bytes>* blocked_sources = nullptr)
      -> Source::Record*;
  auto load_import(
      Context& context,
      Source::Record& owner,
      const Ttx::Dialect::Source::Import& import,
      Bool private_source,
      Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes>&
          resolving,
      const Perimortem::Memory::Dynamic::Vector<
          Perimortem::Memory::Dynamic::Bytes>* blocked_sources)
      -> Source::Record*;
  auto parse_source(Context& context, Source::Record& record) -> Bool;
  auto resolve_imports(
      Context& context,
      Source::Record& record,
      Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes>&
          resolving,
      const Perimortem::Memory::Dynamic::Vector<
          Perimortem::Memory::Dynamic::Bytes>* blocked_sources)
      -> Bool;
  auto update_consumers(
      Context& context,
      Perimortem::Memory::Dynamic::Vector<Snapshot>& snapshots) -> void;
  auto snapshot_consumers(
      Source::Record& record,
      Perimortem::Memory::Dynamic::Vector<Snapshot>& snapshots) -> void;
  auto read_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Memory::Dynamic::Bytes& source_text) -> Bool;
  auto add_error(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes message) -> void;

  Source::Cache sources;
};

}  // namespace Tetrodotoxin::Resolution
