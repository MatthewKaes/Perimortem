// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Swizzle selects and reorders named values from one receiver Pack. A named
// Pack contributes the real producer retained at each selected source index.
// One producer with several results may therefore supply distinct slots. A
// scalar Expression may instead contribute the named Addressables of its
// output Type, in which case each selected slot is a real Address Expression
// bound to that receiver. The result is positional Pack flow and never an
// eagerly materialized Type.
class Swizzle : public Expression {
 public:

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Language::Model::Pack& receiver,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Ttx::Lexical::Anchor anchor) -> Swizzle&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Swizzle"_view);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Language::Model::Pack& {
    return receiver;
  }

  constexpr auto get_projections() const { return projections.get_view(); }

  constexpr auto get_selections() const { return selections.get_view(); }

  // A Type-based Swizzle constructs one Address expression for each selected
  // member. Exposing those Packs preserves their ordinary evaluation edges, so
  // a Terminal can lower the resulting flow without learning how Swizzle found
  // the members. Direct Pack selection keeps this empty because its values
  // remain occurrences of the receiver rather than new expressions.
  constexpr auto get_entries() const
      -> Perimortem::Core::View::Vector<Language::Model::Pack*> override {
    return projection_packs;
  }

 private:
  Swizzle(
      Perimortem::Memory::Allocator::Arena& domain,
      Language::Model::Pack& receiver,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        domain(domain),
        receiver(receiver),
        name_tokens(name_tokens),
        names(names),
        selections(domain),
        projections(domain),
        projection_packs(domain) {}

  Perimortem::Memory::Allocator::Arena& domain;
  Language::Model::Pack& receiver;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Vector<Count> selections;
  Perimortem::Memory::Managed::Vector<const Ttx::Concept::Abstract*>
      projections;
  Perimortem::Memory::Managed::Vector<Language::Model::Pack*>
      projection_packs;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> output;
};

}  // namespace Tetrodotoxin::Library::Language::Access
