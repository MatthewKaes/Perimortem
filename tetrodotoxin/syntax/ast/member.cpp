// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/member.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Ast::Member::parse(Context& ctx) -> Member* {
  return parse(ctx, Class::Type::EndOfStream);
}

auto Ast::Member::parse(Context& ctx, Class::Type scope_type) -> Member* {
  DeclarationPrefix prefix = DeclarationPrefix::parse(ctx);
  return parse(ctx, scope_type, prefix);
}

auto Ast::Member::parse(
    Context& ctx,
    Class::Type,
    const DeclarationPrefix& prefix) -> Member* {
  Definition definition = Definition::parse_header(ctx);
  return parse_declaration(ctx, prefix, definition);
}

auto Ast::Member::parse_declaration(
    Context& ctx,
    const DeclarationPrefix& prefix,
    Definition definition) -> Member* {
  Member& node = ctx.get_arena().allocate<Member>();
  node.disabled = prefix.disabled;
  node.definition = definition;
  node.attributes = prefix.attributes;
  node.documentation = prefix.documentation;

  if (prefix.external) {
    ctx.error("Expected function after 'external'"_view);
  }

  if (!node.definition.is_valid()) {
    return &node;
  }

  if (node.definition.get_form() == Class::Type::Assign) {
    return &node;
  }

  if (ctx.matches(Class::Type::EndStatement)) {
    ctx.advance();
    return &node;
  }

  ctx.require(Class::Type::Assign);
  node.definition.form = Class::Type::Assign;
  if (node.definition.is_alias()) {
    node.definition.alias_target = Type::Ref::parse(ctx);
  } else {
    node.definition.value = Value::parse(ctx);
  }
  ctx.require(Class::Type::EndStatement);
  return &node;
}
