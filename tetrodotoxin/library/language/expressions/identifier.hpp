// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Library::Language::Expressions {

// Identifier is one authored root name Expression. It retains the exact Token
// and a view of the source spelling in the shared transaction Arena, then
// delays binding until the Type defining pass is complete. A Type result stays
// outside value flow while an Addressable result keeps its ordinary value Type.
// Both remain opaque until link selects them.
class Identifier : public Expression {
 public:
  TTX_CONTRACT(Identifier, Expression);

  static auto create_authored(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Ttx::Lexical::Token token,
      Ttx::Lexical::Anchor anchor) -> Identifier& {
    Perimortem::Core::View::Bytes name =
        token.caculate_text(cursor.get_source_text());
    return Expression::create_authored<Identifier>(
        cursor.get_arena(), anchor, [&](auto authored) -> Identifier {
          return Identifier(token, name, lexical_context, authored);
        });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name) -> Identifier& {
    return Expression::create_synthetic<Identifier>(
        arena, [&](auto source) -> Identifier {
          return Identifier(
              {}, name, Ttx::Concept::Unknown::get_unknown(), source);
        });
  }

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto link_restored(
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto get_result() const -> const Ttx::Concept::Abstract& override;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;
  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  // A malformed following operator may keep this Identifier outside a
  // retained Statement. Its authored context can still answer the strongest
  // source ordered binding without manufacturing a completed result edge.
  auto resolve_authored() const -> const Ttx::Concept::Abstract&;

  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

 private:
  constexpr Identifier(
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        token(token),
        name(name),
        lexical_context(&lexical_context) {}

  Ttx::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Abstract* lexical_context;
  Perimortem::Core::Option<const Ttx::Concept::Abstract*> result;
};

}  // namespace Tetrodotoxin::Library::Language::Expressions
