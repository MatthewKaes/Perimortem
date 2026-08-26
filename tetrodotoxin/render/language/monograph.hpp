// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/render/language/declarations.hpp"

namespace Tetrodotoxin::Render::Language {

// Monograph is the retained root of one Render contract source. Each concrete
// declaration enters the operator domain that can resolve it, while ordered
// views preserve the exact identities used by Shader and editor tooling.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context) -> Monograph&;

  auto retain_addressable(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_callable(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_type(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto compose(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored() -> Bool override;

  auto compose_restored() -> Bool override;

  auto finalize_restored() -> Bool override;

  auto resolve_context(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_local_context(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_addressables() const {
    return declarations.get_addressables();
  }

  constexpr auto get_callables() const { return declarations.get_callables(); }

  constexpr auto get_types() const { return declarations.get_types(); }

  constexpr auto is_finalized() const -> Bool { return finalized; }

  TTX_NAME("Render"_view);

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context)
      : Tetrodotoxin::Language::Monograph(
            arena,
            language,
            documentation,
            context),
        declarations(arena) {}

  Declarations declarations;
  Bool finalized = False;
};

}  // namespace Tetrodotoxin::Render::Language
