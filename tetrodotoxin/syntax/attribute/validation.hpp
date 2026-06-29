// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/dialect/dialect.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

namespace Tetrodotoxin::Syntax::Attribute {

auto validate_package(
    Context& ctx,
    const Dialect::Dialect& dialect,
    Perimortem::Core::View::Vector<Ast::Import> imports,
    Perimortem::Core::View::Vector<Ast::Member*> members,
    Perimortem::Core::View::Vector<Ast::Function*> functions,
    Perimortem::Core::View::Vector<Type::Declaration*> types) -> void;

}  // namespace Tetrodotoxin::Syntax::Attribute
