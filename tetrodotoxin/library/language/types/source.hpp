// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/types/composite.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Source is one Library Monograph's synthetic root Composite. It owns synthetic
// Definition policy, source grammar, and Static publication while Composite
// keeps the shared inventories, lookup, and lifecycle.
class Source : public Composite {
 private:
  Source(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Composite(domain, definition) {}

 protected:
  auto retain_binding(Ttx::Concept::Abstract& binding, Category category)
      -> Bool override;

 public:
  TTX_CONTRACT(Source, Composite, 0xa972070bd27746e0, 0x959dd29d7924aed4);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Monograph& host,
      const Ttx::Lexical::Anchor& source_anchor) -> Source&;

  Source(const Source&) = delete;
  Source(Source&&) = delete;
  auto operator=(const Source&) -> Source& = delete;
  auto operator=(Source&&) -> Source& = delete;

  // Source consumes its extension forms and delegates every ordinary Definition
  // to Composite. The root is therefore the only place that can admit using
  // without teaching Composite about source grammar.
  auto parse(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto bind_static(Ttx::Concept::Abstract& binding, Category category) -> Bool;

  auto can_bind_static(const Ttx::Concept::Abstract& binding, Category category)
      const -> Bool;

  constexpr auto resolve() const -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
};

}  // namespace Tetrodotoxin::Library::Language::Types
