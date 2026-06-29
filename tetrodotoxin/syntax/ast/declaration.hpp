// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Ast {

struct DeclarationPrefix {
  static auto parse(Context& ctx) -> DeclarationPrefix;

  Comment documentation;
  Perimortem::Core::View::Vector<Attribute> attributes;
  Bool disabled = False;
  Bool external = False;
};

}  // namespace Tetrodotoxin::Syntax::Ast
