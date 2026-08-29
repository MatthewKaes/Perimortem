// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Type is one postfix `:: Name` Expression. It retains the receiver and exact
// authored Token without binding during parsing. Linking evaluates the
// receiver result and selects the next context through that owner. Intermediate
// Package, Monograph, and namespace contexts remain available to another `::`.
// A terminal Type can enter Static invocation or declaration flow, while a
// context owned value such as an Enumeration case enters ordinary value flow.
class Type : public Expression {
 public:
  TTX_CONTRACT(Type, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Ttx::Lexical::Anchor anchor) -> Type&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_result() const -> const Ttx::Concept::Abstract& override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  // A following incomplete postfix may leave this access outside a retained
  // Statement. The receiver still owns enough authored context to answer the
  // strongest currently available selection without completing this node.
  auto resolve_authored() const -> const Ttx::Concept::Abstract&;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }
  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

 private:
  constexpr Type(
      Model::Pack& receiver,
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), token(token), name(name) {}

  Model::Pack& receiver;
  Ttx::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<const Ttx::Concept::Abstract*> selected;
};

}  // namespace Tetrodotoxin::Library::Language::Access
