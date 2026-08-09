// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/environment/origin.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Environment {

// Owns retained Monograph lifetime, authored provenance, and discovery order.
// One staged range freezes before linking so every link precedes every
// finalizer and no hook can run twice.
class Retention {
 public:
  explicit Retention(Perimortem::Memory::Allocator::Arena& arena);
  ~Retention();

  auto retain(
      Language::Monograph& monograph,
      Perimortem::Core::Option<Origin> origin) -> Bool;
  auto get_size() const -> Count;
  auto get_monograph(Count index) const -> Language::Monograph&;
  auto get_origin(Count index) const -> Perimortem::Core::Option<Origin>;
  auto find_origin(const Language::Monograph& monograph) const
      -> Perimortem::Core::Option<Origin>;
  auto has_staged() const -> Bool;
  auto awaits_finalize() const -> Bool;
  auto link(Ttx::Lexical::Errors& errors) -> Bool;
  auto finalize(Ttx::Lexical::Errors& errors) -> Bool;
  auto abandon() -> void;

 private:
  class Entry {
   public:
    Entry(
        Language::Monograph& monograph,
        Perimortem::Core::Option<Origin> origin);

    auto get_monograph() const -> Language::Monograph&;
    auto get_origin() const -> Perimortem::Core::Option<Origin>;
    auto get_next_diagnostic() const -> Count;
    auto consume_diagnostics(Count count) -> void;

   private:
    Language::Monograph& monograph;
    Perimortem::Core::View::Bytes origin_path;
    Perimortem::Core::View::Bytes origin_body;
    Ttx::Lexical::Span origin_span;
    Count next_diagnostic;
    Bool has_origin;
  };

  enum class Stage : Unsigned_8 {
    Staging,
    Linked,
  };

  auto render_diagnostics(Ttx::Lexical::Errors& errors, Entry& entry) -> void;
  auto consume_range() -> void;

  Perimortem::Memory::Managed::Vector<Entry> entries;
  Count range_start;
  Count range_end;
  Stage stage;
};

}  // namespace Tetrodotoxin::Environment
