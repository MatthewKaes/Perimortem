// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Render::Language {

// Declarations owns the three namespaces shared by Render roots and nested
// Structures. Every declaration is retained once in source order, while one
// visibility fact answers public queries without maintaining parallel vectors.
// This is Render policy rather than a generic language Scope because another
// language may define different namespaces, collisions, or publication rules.
class Declarations {
 public:
  constexpr Declarations(Perimortem::Memory::Allocator::Arena& domain)
      : authority(*this),
        published(domain),
        addressables(domain),
        callables(domain),
        types(domain) {}

  auto retain_addressable(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;
  auto retain_callable(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;
  auto retain_type(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto link(Ttx::Lexical::Cursor& cursor, Ttx::Concept::Abstract& context)
      -> Bool;
  auto link_restored(Ttx::Concept::Abstract& context) -> Bool;

  constexpr auto get_authority() const -> const Ttx::Concept::Abstract& {
    return authority;
  }

  auto visit_concepts(ttx_named_abstract_callable* visitor) const -> void;

  static auto resolve_lexical_context(
      const Ttx::Concept::Abstract& context,
      Perimortem::Core::View::Bytes name) -> const Ttx::Concept::Abstract&;

  auto resolve_addressable(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_callable(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_type(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_addressables() const { return addressables.get_view(); }
  constexpr auto get_callables() const { return callables.get_view(); }
  constexpr auto get_types() const { return types.get_view(); }
  constexpr auto is_linked() const -> Bool { return linked; }

 private:
  class Authority : public Ttx::Concept::Abstract {
   public:
    constexpr explicit Authority(const Declarations& owner) : owner(owner) {}

    TTX_CONTRACT(Authority, Ttx::Concept::Abstract);
    TTX_NAME("static"_view);
    TTX_EMPTY_DOCUMENTATION();

    auto resolve_concept(Perimortem::Core::View::Bytes name) const
        -> const Ttx::Concept::Abstract& override;
    auto visit_concepts(ttx_named_abstract_callable* visitor) const
        -> void override;

   private:
    const Declarations& owner;
  };

  auto retain(
      Perimortem::Memory::Managed::Vector<Ttx::Concept::Abstract*>&
          declarations,
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;
  auto resolve(
      Perimortem::Core::View::Vector<Ttx::Concept::Abstract*> declarations,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::Visibility visibility) const
      -> const Ttx::Concept::Abstract&;

  Authority authority;
  Perimortem::Memory::Managed::Map<const Ttx::Concept::Abstract*, Bool>
      published;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Abstract*> addressables;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Abstract*> callables;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Abstract*> types;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Render::Language
