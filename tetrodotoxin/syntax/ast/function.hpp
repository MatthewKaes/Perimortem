// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/ast/declaration.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/ast/layout.hpp"
#include "tetrodotoxin/syntax/ast/param.hpp"
#include "tetrodotoxin/syntax/ast/statement.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// The RHS of a function definition: Layout -> Layout { body }
// or Layout -> Layout ; for signature-only members.
class Function {
 public:
  static auto parse(Context& ctx, Bool external = False) -> Function;
  static auto starts(Context& ctx) -> Bool;
  static auto parse_declaration(
      Context& ctx,
      Lexical::Class::Type scope_type,
      const DeclarationPrefix& prefix) -> Function*;

  auto get_documentation() const -> const Comment& { return documentation; }
  auto get_attributes() const -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }
  auto get_definition() const -> const Definition& { return definition; }
  auto get_params() const -> const Layout& { return params; }
  auto get_returns() const -> const Layout& { return returns; }
  auto get_body() const -> Perimortem::Core::View::Vector<Statement*> {
    return body;
  }
  auto has_body() const -> Bool { return body_present; }
  auto is_external() const -> Bool { return external; }
  auto is_disabled() const -> Bool { return disabled; }

 private:
  Comment documentation;
  Perimortem::Core::View::Vector<Attribute> attributes;
  Definition definition;
  Layout params;
  Layout returns;
  Perimortem::Core::View::Vector<Statement*> body;
  Bool body_present = False;
  Bool external = False;
  Bool disabled = False;
};

}  // namespace Tetrodotoxin::Syntax::Ast
