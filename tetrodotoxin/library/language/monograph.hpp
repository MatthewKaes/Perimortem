// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"

namespace Tetrodotoxin::Library::Language {

// Monograph retains one Library source transaction and its intrinsic root
// vocabulary. The synthetic Source owns source grammar and graph completion.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context) -> Monograph&;

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context) -> Monograph&;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored() -> Bool override;

  auto finalize_restored() -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  constexpr auto get_root() const -> const Ttx::Concept::Abstract& override {
    return source;
  }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  // Resolves only this source's authored, intrinsic, using, and common Import
  // surface. An outer Dialect can expose that surface without re-entering its
  // own context fallback.
  auto resolve_local_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_lexical_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto can_bind_source_type(Perimortem::Core::View::Bytes name) const -> Bool;

  auto retain_import(
      const Tetrodotoxin::Language::Import::Description& description,
      Perimortem::Core::Option<Ttx::Lexical::Associations&> associations = {})
      -> Bool override;

  constexpr auto get_source() -> Types::Source& { return source; }

  constexpr auto get_source() const -> const Types::Source& { return source; }

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context);

  auto resolve_root_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Ttx::Concept::Abstract&>
          vocabulary;
  Types::Source& source;
};

}  // namespace Tetrodotoxin::Library::Language
