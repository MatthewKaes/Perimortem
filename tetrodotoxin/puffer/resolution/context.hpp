// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/object.hpp"

#include "tetrodotoxin/puffer/resolution/source/record.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Puffer::Resolution {

// Per-request source lifetime and diagnostic surface.
//
// Source::Cache owns published records, while Context retains the records
// returned during this request. That keeps a caller's raw Record address alive
// if a later update removes it from the cache. Each Record already owns its
// arena, so Context does not retain transaction state separately. Failed
// diagnostics are migrated before their cursor and transaction disappear.
// Compilation rooted in this request uses the same sink, which gives callers
// one result surface for resolver, ISA, compiler, and target errors.
class Context {
 public:
  Context() : errors(error_arena) {}

  auto adopt_record(Perimortem::Memory::Dynamic::Object<Source::Record> record)
      -> Perimortem::Memory::Dynamic::Object<Source::Record>;

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
  // Long-lived services receive the sink rather than storing another error
  // collection and forcing callers to arbitrate between competing results.
  constexpr auto get_error_sink() -> Ttx::Lexical::Errors& { return errors; }
  constexpr auto get_errors() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Errors::Error> {
    return errors;
  }

 private:
  auto persist_error(const Ttx::Lexical::Errors::Error& error) -> void;
  auto persist(Perimortem::Core::View::Bytes bytes)
      -> Perimortem::Core::View::Bytes;
  auto persist(const Ttx::Lexical::Token& token) -> const Ttx::Lexical::Token&;

  Perimortem::Memory::Allocator::Arena error_arena;
  Ttx::Lexical::Errors errors;
  Perimortem::Memory::Dynamic::
      Map<Source::Record*, Perimortem::Memory::Dynamic::Object<Source::Record>>
          record_handles;
};

}  // namespace Tetrodotoxin::Puffer::Resolution
