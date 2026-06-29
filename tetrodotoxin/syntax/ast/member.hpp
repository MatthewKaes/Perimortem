// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/declaration.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// A top-level or scope-level member. Wraps a Definition with optional
// preceding attributes and an optional Disabled (`/>`) modifier.
//
// A Disabled member is semantically equivalent to `@if(false) { definition }`.
class Member {
 public:
  static auto parse(Context& ctx) -> Member*;
  static auto parse(Context& ctx, Lexical::Class::Type scope_type) -> Member*;
  static auto parse(
      Context& ctx,
      Lexical::Class::Type scope_type,
      const DeclarationPrefix& prefix) -> Member*;
  static auto parse_declaration(
      Context& ctx,
      const DeclarationPrefix& prefix,
      Definition definition) -> Member*;

  auto get_documentation() const -> const Comment& { return documentation; }

  auto get_attributes() const -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

  auto get_definition() const -> const Definition& { return definition; }
  auto is_disabled() const -> Bool { return disabled; }

 private:
  Comment documentation;
  Perimortem::Core::View::Vector<Attribute> attributes;
  Definition definition;
  Bool disabled = False;
};

}  // namespace Tetrodotoxin::Syntax::Ast
