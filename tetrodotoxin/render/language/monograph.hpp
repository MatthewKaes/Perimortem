// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/concept/reference.hpp"

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

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

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

  constexpr auto get_addressables() const { return addressables.get_view(); }

  constexpr auto get_callables() const { return callables.get_view(); }

  constexpr auto get_types() const { return types.get_view(); }

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
        addressables(arena),
        published_addressables(arena),
        callables(arena),
        published_callables(arena),
        types(arena),
        published_types(arena) {}

  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      addressables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      published_addressables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      callables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      published_callables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      types;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      published_types;
  Bool linked = False;
  Bool finalized = False;
};

}  // namespace Tetrodotoxin::Render::Language
