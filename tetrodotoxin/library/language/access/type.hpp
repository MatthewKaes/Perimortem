// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Type is one postfix `:: Name` Expression. It retains the receiver and exact
// authored Token without binding during parsing. Linking evaluates the
// receiver result and selects the next context through that owner. Intermediate
// Package, Monograph, and namespace contexts remain available to another `::`,
// while a terminal Type can enter Static invocation or declaration flow.
class Type : public Expression {
 public:
  TTX_CONTRACT(Type, Expression);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_result() const -> const Ttx::Concept::Abstract& override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  auto lower(Llvm::Builder& body) const -> Bool override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

 private:
  constexpr Type(
      Expression& receiver,
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), token(token), name(name) {}

  Expression& receiver;
  Ttx::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      selected;
};

}  // namespace Tetrodotoxin::Library::Language::Access
