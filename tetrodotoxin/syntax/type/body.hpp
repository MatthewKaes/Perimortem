// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Ast {

class Function;
class Member;

}  // namespace Tetrodotoxin::Syntax::Ast

namespace Tetrodotoxin::Syntax::Type {

class Declaration;
class Ref;

class Body {
 public:
  static auto parse(
      Context& ctx,
      Lexical::Class::Type scope_type,
      Lexical::Class::Type end_token) -> Body;
  static auto from_members(Perimortem::Core::View::Vector<Ast::Member*> members)
      -> Body;
  static auto from_parts(
      Perimortem::Core::View::Vector<Ast::Member*> members,
      Perimortem::Core::View::Vector<Ast::Function*> functions,
      Perimortem::Core::View::Vector<Declaration*> types) -> Body;

  auto get_members() const -> Perimortem::Core::View::Vector<Ast::Member*> {
    return members;
  }
  auto get_functions() const -> Perimortem::Core::View::Vector<Ast::Function*> {
    return functions;
  }
  auto get_types() const -> Perimortem::Core::View::Vector<Declaration*> {
    return types;
  }

 private:
  Perimortem::Core::View::Vector<Ast::Member*> members;
  Perimortem::Core::View::Vector<Ast::Function*> functions;
  Perimortem::Core::View::Vector<Declaration*> types;
};

}  // namespace Tetrodotoxin::Syntax::Type
