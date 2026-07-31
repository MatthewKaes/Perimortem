// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/environment/origin.hpp"
#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Owns retained Monograph lifetime, authored provenance, and discovery order.
// Completion advances one durable prefix so later Workspace imports cannot run
// a concrete post pass twice.
class Retention {
 public:
  explicit Retention(Perimortem::Memory::Allocator::Arena& arena);
  ~Retention();

  auto retain(
      Language::Dialect::Monograph& monograph,
      Perimortem::Utility::Option<Origin> origin) -> void;
  auto get_size() const -> Count;
  auto get_monograph(Count index) const -> Language::Dialect::Monograph&;
  auto get_origin(Count index) const -> Perimortem::Utility::Option<Origin>;
  auto find_origin(const Language::Dialect::Monograph& monograph) const
      -> Perimortem::Utility::Option<Origin>;
  auto complete(Ttx::Lexical::Errors& errors) -> Bool;

 private:
  class Entry {
   public:
    Entry(
        Language::Dialect::Monograph& monograph,
        Perimortem::Utility::Option<Origin> origin);

    auto get_monograph() const -> Language::Dialect::Monograph&;
    auto get_origin() const -> Perimortem::Utility::Option<Origin>;

   private:
    Language::Dialect::Monograph& monograph;
    Perimortem::Core::View::Bytes origin_path;
    Perimortem::Core::View::Bytes origin_body;
    Ttx::Lexical::Span origin_span;
    Bool has_origin;
  };

  Perimortem::Memory::Managed::Vector<Entry> entries;
  Count next_to_complete;
};

}  // namespace Tetrodotoxin::Environment
