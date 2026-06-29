// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/definition.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Ast::Definition::is_alias() const -> Bool {
  if (type_ref.get_segments().is_empty()) {
    return False;
  }

  return type_ref.get_segments()[0].klass == Class::Type::Alias;
}

auto Ast::Definition::parse_header(Context& ctx) -> Definition {
  Definition result;

  if (ctx.get_current().get_class().is_sigil()) {
    result.sigil = ctx.get_current().get_class();
    ctx.advance();
  }

  if (!ctx.matches(Class::Type::Addressable) &&
      !ctx.matches(Class::Type::Type)) {
    ctx.error(
        "Expected a declaration name"_view,
        "Names begin with a letter, e.g. '@hidden width : Bits_32 = 0;'"_view);
    ctx.advance();
    return result;
  }

  result.name = ctx.get_current().get_text();
  ctx.advance();

  ctx.require(Class::Type::Define);
  result.type_ref = Type::Ref::parse(ctx);
  return result;
}

auto Ast::Definition::parse(Context& ctx) -> Definition {
  Definition result = Definition::parse_header(ctx);

  if (!result.is_valid()) {
    return result;
  }

  if (ctx.matches(Class::Type::EndStatement)) {
    ctx.advance();
    result.form = Class::Type::EndStatement;
    return result;
  }

  ctx.require(Class::Type::Assign);
  result.form = Class::Type::Assign;

  if (result.is_alias()) {
    result.alias_target = Type::Ref::parse(ctx);
  } else {
    result.value = Value::parse(ctx);
  }

  ctx.require(Class::Type::EndStatement);
  return result;
}
