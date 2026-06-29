// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/function.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Ast::Function::parse(Context& ctx, Bool external) -> Function {
  Function result;
  result.external = external;

  result.params = Layout::parse(ctx);

  if (result.params.is_unnamed() ||
      !result.params.get_value_type().is_empty()) {
    ctx.error(
        "Expected named function parameters"_view,
        "Function parameters use a layout like '[.source : View[Bytes]]'"_view);
  }

  ctx.require(Class::Type::CallOp);
  result.returns = Layout::parse(ctx);

  if (ctx.matches(Class::Type::EndStatement)) {
    ctx.advance();
    return result;
  }

  if (external) {
    ctx.error("Expected ';' after external function declaration"_view);
  }

  // Function body: { statements... }
  ctx.require(Class::Type::ScopeStart);
  Managed::Vector<Statement*> stmt_list(ctx.get_arena());

  while (!ctx.matches(Class::Type::ScopeEnd) && !ctx.is_done()) {
    Statement* stmt = Statement::parse(ctx);

    if (stmt) {
      stmt_list.insert(stmt);
    }
  }

  ctx.require(Class::Type::ScopeEnd);
  result.body = stmt_list.get_view();
  result.body_present = True;
  return result;
}

auto Ast::Function::starts(Context& ctx) -> Bool {
  return ctx.matches(Class::Type::Func) ||
         (ctx.get_current().get_class().is_sigil() &&
          ctx.get_peek().get_class() == Class::Type::Func);
}

auto Ast::Function::parse_declaration(
    Context& ctx,
    Class::Type scope_type,
    const DeclarationPrefix& prefix) -> Function* {
  Function& result = ctx.get_arena().allocate<Function>();
  result.documentation = prefix.documentation;
  result.attributes = prefix.attributes;
  result.definition = Definition();
  result.params = Layout();
  result.returns = Layout();
  result.body = View::Vector<Statement*>();
  result.body_present = False;
  result.external = False;
  result.disabled = prefix.disabled;

  if (prefix.external && scope_type != Class::Type::Foreign) {
    ctx.error("External functions are only valid in Foreign blocks"_view);
  }

  Bool has_function_sigil = ctx.get_current().get_class().is_sigil() &&
                            ctx.get_peek().get_class() == Class::Type::Func;

  if (has_function_sigil) {
    result.definition.sigil = ctx.get_current().get_class();
    ctx.advance();
  }

  ctx.require(Class::Type::Func);

  if (!ctx.matches(Class::Type::Addressable)) {
    ctx.error(
        "Expected a function name"_view,
        "Function names are addressables, e.g. '@public func main[...]'"_view);
    return &result;
  }

  result.definition.name = ctx.get_current().get_text();
  ctx.advance();

  Function signature = Function::parse(ctx, prefix.external);
  result.params = signature.params;
  result.returns = signature.returns;
  result.body = signature.body;
  result.body_present = signature.body_present;
  result.external = signature.external;

  if (!result.has_body() && !result.is_external()) {
    ctx.error(
        "Expected function body"_view,
        "Only external functions inside Foreign blocks can end with ';'"_view);
  }

  return &result;
}
