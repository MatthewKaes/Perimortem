// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/object.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "tetrodotoxin/puffer/resolution/source/cache.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/lexical/errors.hpp"

namespace Ttx::Lexical {
class Cursor;
}

namespace Tetrodotoxin::Puffer::Resolution {

// Resolves TTX source requests into cached source records.
//
// Resolver is the source resolution state machine for one package boundary in a
// caller-provided Tetrodotoxin toolchain. Tools such as the CLI and LSP create
// a root resolver from their active toolchain. Packages can be represented by
// their package source and may later grow their own local resolver for private
// package files. Public lookup still goes through package names rather than
// private source paths.
//
// Only valid resolved sources are cached. If loading or updating a source would
// leave an evaluation error, missing import, ISA mismatch, package failure, or
// cycle in the graph, that source is rejected from the cache and any cached
// consumers made stale by the failed update are removed.
class Resolver {
 public:
  // Per-request diagnostic sink.
  //
  // The resolver owns source records and diagnostics that survive a source-load
  // transaction. Cursor errors point into tokenizer or record memory, so the
  // context keeps request-created records alive until the caller finishes
  // consuming diagnostics.
  class Context {
   public:
    Context() : errors(error_arena) {}

    // Request-created records arrive as dynamic handles. The context keeps a
    // handle alive until diagnostics are consumed. The cache keeps its own
    // handle when a valid record is published.
	    auto adopt_record(
	        Perimortem::Memory::Dynamic::Object<Source::Record> record)
	        -> Perimortem::Memory::Dynamic::Object<Source::Record>;

    // Error metadata is migrated into this context, but source text is not
    // copied. Source views must come from retained records or caller-owned
    // storage that outlives the context.
    auto persist_errors(
        Perimortem::Core::View::Vector<Ttx::Lexical::Errors::Error> source)
        -> void;
    auto persist_errors(const Ttx::Lexical::Errors::Error& error) -> void {
      persist_error(error);
    }
    auto persist_errors(const Ttx::Lexical::Errors& source) -> void {
      persist_errors(source.get_view());
    }

    constexpr auto has_errors() const -> Bool { return errors.has_errors(); }
    constexpr auto get_errors() const
        -> Perimortem::Core::View::Vector<Ttx::Lexical::Errors::Error> {
      return errors;
    }

   private:
    auto persist_error(const Ttx::Lexical::Errors::Error& error) -> void;
    auto persist(Perimortem::Core::View::Bytes bytes)
        -> Perimortem::Core::View::Bytes;
    auto persist(const Ttx::Lexical::Token& token)
        -> const Ttx::Lexical::Token&;

    Perimortem::Memory::Allocator::Arena error_arena;
    Ttx::Lexical::Errors errors;
    Perimortem::Memory::Dynamic::Map<
        Source::Record*,
        Perimortem::Memory::Dynamic::Object<Source::Record>>
        record_handles;
  };

  explicit Resolver(const Tetrodotoxin::Toolchain& toolchain)
      : toolchain(toolchain) {}

  auto load_source(Context& context, Perimortem::Core::View::Bytes source_path)
      -> Source::Record*;
  auto load_source(
      Context& context,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes ttx_content) -> Source::Record*;
  auto resolve(Perimortem::Core::View::Bytes import_name) -> Source::Record*;

  template <typename visitor_type>
  auto visit_reachable(Source::Record& root, visitor_type visit) const -> void {
    visit(root);
    sources.visit_producers(root, visit);
  }

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
      const Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>*
          blocked_sources = nullptr)
      -> Source::Record*;
  auto load_import(
      Context& context,
      Ttx::Lexical::Cursor& cursor,
      Source::Record& owner,
      const Tetrodotoxin::Puffer::Isa::Boot::Import& import,
      Bool private_source,
      Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes>&
          resolving,
      const Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>*
          blocked_sources)
      -> Source::Record*;
  auto evaluate_boot(Ttx::Lexical::Cursor& cursor)
      -> Tetrodotoxin::Puffer::Isa::Boot::Envelope*;
  auto execute_body(
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Puffer::Isa::Boot::Envelope& boot,
      Perimortem::Core::View::Vector<Source::Record*> producers) -> Ttx::Type*;
  auto resolve_imports(
      Context& context,
      Ttx::Lexical::Cursor& cursor,
      Source::Record& record,
      Perimortem::Memory::Dynamic::Vector<Source::Record*>& producers,
      Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes>&
          resolving,
      const Perimortem::Memory::Dynamic::Set<Perimortem::Core::View::Bytes>*
          blocked_sources) -> Bool;
  auto update_consumers(
      Context& context,
      Perimortem::Memory::Dynamic::Vector<Snapshot>& snapshots) -> void;
  auto snapshot_consumers(
      Source::Record& record,
      Perimortem::Memory::Dynamic::Vector<Snapshot>& snapshots) -> void;
  auto read_source(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Memory::Dynamic::Bytes& source_text) -> Bool;

  Source::Cache sources;
  const Tetrodotoxin::Toolchain& toolchain;
};

}  // namespace Tetrodotoxin::Puffer::Resolution
