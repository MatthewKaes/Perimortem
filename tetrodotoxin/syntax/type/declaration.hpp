// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/declaration.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/type/body.hpp"

namespace Tetrodotoxin::Syntax::Expression {
class Expression;
}

namespace Tetrodotoxin::Syntax::Ast {
class Function;
class Member;
}  // namespace Tetrodotoxin::Syntax::Ast

namespace Tetrodotoxin::Syntax::Type {

class Declaration {
 public:
  static auto is_type_definition(const Ast::Definition& definition) -> Bool;
  static auto parse_declaration(
      Context& ctx,
      Lexical::Class::Type scope_type,
      const Ast::DeclarationPrefix& prefix,
      Ast::Definition definition) -> Declaration*;

  auto get_documentation() const -> const Ast::Comment& {
    return documentation;
  }
  auto get_attributes() const
      -> Perimortem::Core::View::Vector<Ast::Attribute> {
    return attributes;
  }
  auto get_definition() const -> const Ast::Definition& { return definition; }
  auto get_kind() const -> DeclarationKind { return kind; }
  auto get_scope_type() const -> Lexical::Class::Type {
    return to_scope_type(kind);
  }
  auto get_body() const -> const Body& { return body; }
  auto is_disabled() const -> Bool { return disabled; }
  auto is_scoped_body() const -> Bool;

  auto get_scope() const -> Perimortem::Core::View::Vector<Ast::Member*> {
    return body.get_members();
  }
  auto get_scope_functions() const
      -> Perimortem::Core::View::Vector<Ast::Function*> {
    return body.get_functions();
  }
  auto get_scope_types() const -> Perimortem::Core::View::Vector<Declaration*> {
    return body.get_types();
  }
  auto find_member(Perimortem::Core::View::Bytes name) const
      -> const Ast::Member*;
  auto find_function(Perimortem::Core::View::Bytes name) const
      -> const Ast::Function*;
  auto get_visible_member_count() const -> Count;
  auto get_visible_member(Count index) const -> const Ast::Member*;

 private:
  static auto parse_enum_members(Context& ctx, const Ref& type)
      -> Perimortem::Core::View::Vector<Ast::Member*>;
  static auto parse_enum_scope(Context& ctx, const Ref& type) -> Body;
  static auto make_enum_definition(
      Perimortem::Core::View::Bytes name,
      const Ref& storage_type,
      Expression::Expression* value) -> Ast::Definition;

  Ast::Comment documentation;
  Perimortem::Core::View::Vector<Ast::Attribute> attributes;
  Ast::Definition definition;
  Body body;
  DeclarationKind kind = DeclarationKind::Invalid;
  Bool disabled = False;
};

}  // namespace Tetrodotoxin::Syntax::Type
