// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Language::Model {

class Pack;
class Type;

// Initialization owns Library's construction relationship for one Domain.
// The receiving language decides how empty or supplied Pack flow becomes a
// value, while TTX Domain remains limited to identity and Layout projection.
class Initialization : public Ttx::Concept::Abstract {
 public:
  TTX_NAME("initialization"_view);
  TTX_EMPTY_DOCUMENTATION();

  virtual auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Pack&> = 0;

  virtual auto create_supplied(
      Ttx::Lexical::Cursor& cursor,
      Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Pack&> = 0;

  virtual auto create_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Pack&> = 0;
};

template <typename Owner>
class OwnedInitialization final : public Initialization {
 public:
  explicit OwnedInitialization(const Owner& owner) : owner(owner) {}

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Pack&> override {
    return owner.initialize_default(arena);
  }

  auto create_supplied(
      Ttx::Lexical::Cursor& cursor,
      Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Pack&> override {
    if constexpr (requires {
                    owner.initialize_supplied(
                        cursor, source, access_scope, anchor);
                  }) {
      return owner.initialize_supplied(cursor, source, access_scope, anchor);
    }
    cursor.create_expression_error(
        anchor,
        "Selected Domain does not accept supplied initializer values."_view,
        "Omit the argument list to request its default initialization."_view);
    return {};
  }

  auto create_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Pack&> override {
    if constexpr (requires {
                    owner.initialize_supplied_restored(
                        arena, source, access_scope);
                  }) {
      return owner.initialize_supplied_restored(arena, source, access_scope);
    }
    return {};
  }

 private:
  const Owner& owner;
};

auto initialize_default(
    const Type& target,
    Perimortem::Memory::Allocator::Arena& arena)
    -> Perimortem::Core::Option<Pack&>;

auto initialize_supplied(
    const Type& target,
    Ttx::Lexical::Cursor& cursor,
    Pack& source,
    Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope,
    Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
    -> Perimortem::Core::Option<Pack&>;

auto initialize_supplied_restored(
    const Type& target,
    Perimortem::Memory::Allocator::Arena& arena,
    Pack& source,
    Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
    -> Perimortem::Core::Option<Pack&>;

}  // namespace Tetrodotoxin::Library::Language::Model
