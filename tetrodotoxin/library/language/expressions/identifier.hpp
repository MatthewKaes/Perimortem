// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
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
      Ttx::Lexical::Token token,
      Ttx::Lexical::Anchor anchor) -> Identifier& {
    Perimortem::Core::View::Bytes name =
        token.caculate_text(cursor.get_source_text());
    return Expression::create_authored<Identifier>(
        cursor.get_arena(), anchor, [&](auto authored) -> Identifier {
          return Identifier(token, name, authored);
        });
  }

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto get_result() const -> const Ttx::Concept::Abstract& override;

  auto lower(Llvm::Builder& body) const -> Bool override;

  auto lower_write_target(Llvm::Builder& body) const -> Bool override;

  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

 private:
  constexpr Identifier(
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), token(token), name(name) {}

  Ttx::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      result;
};

}  // namespace Tetrodotoxin::Library::Language::Expressions
