// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/statement.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/ast/layout.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/expression/addressable.hpp"
#include "tetrodotoxin/syntax/expression/expression.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

class Ast::Statement::Parser {
 public:
  static auto parse(Context& ctx) -> Statement*;

 private:
  static auto alloc(Context& ctx, Class klass = Class::Type::EndOfStream)
      -> Statement*;
  static auto parse_func_body(Context& ctx) -> View::Vector<Statement*>;
  static auto parse_required_pack(Context& ctx, View::Bytes context)
      -> Expression::Expression*;
  static auto parse_if_like(Context& ctx, Class keyword) -> Statement*;
  static auto parse_if(Context& ctx) -> Statement*;
  static auto parse_for(Context& ctx) -> Statement*;
  static auto parse_while(Context& ctx) -> Statement*;
  static auto parse_match(Context& ctx) -> Statement*;
  static auto parse_return(Context& ctx) -> Statement*;
  static auto parse_break(Context& ctx) -> Statement*;
  static auto parse_continue(Context& ctx) -> Statement*;
  static auto matches_assignment_op(Class type) -> Bool;
  static auto parse_declaration(Context& ctx) -> Statement*;
  static auto parse_scope_statement(Context& ctx) -> Statement*;
  static auto is_assignable_base(const Expression::Expression* expr) -> Bool;
  static auto is_assignable_root(const Expression::Expression* expr) -> Bool;
  static auto is_assignable_address(const Expression::Expression* expr) -> Bool;
  static auto parse_assignment_or_expr_statement(Context& ctx) -> Statement*;
};

Ast::Statement::Statement(Class klass) : klass(klass) {}

auto Ast::Statement::Parser::alloc(Context& ctx, Class klass) -> Statement* {
  Class node_class =
      klass == Class::Type::EndOfStream ? ctx.get_current().get_class() : klass;
  return new (ctx.get_arena().allocate(sizeof(Statement)))
      Statement(node_class);
}

auto Ast::Statement::Parser::parse_func_body(Context& ctx)
    -> View::Vector<Statement*> {
  Managed::Vector<Statement*> stmts(ctx.get_arena());

  ctx.require(Class::Type::ScopeStart);

  while (!ctx.matches(Class::Type::ScopeEnd) && !ctx.is_done()) {
    Statement* stmt = Statement::parse(ctx);

    if (stmt) {
      stmts.insert(stmt);
    }
  }

  ctx.require(Class::Type::ScopeEnd);
  return stmts.get_view();
}

auto Ast::Statement::Parser::parse_required_pack(
    Context& ctx,
    View::Bytes context) -> Expression::Expression* {
  if (!ctx.matches(Class::Type::PackingStart)) {
    ctx.error("Expected a pack"_view, context);
    return Expression::Expression::parse(ctx);
  }

  Expression::Expression* pack = Expression::Expression::parse(ctx);

  if (pack && pack->get_class() != Class::Type::PackingStart) {
    ctx.error("Expected a pack"_view, context);
  }

  return pack;
}

auto Ast::Statement::Parser::parse_if_like(Context& ctx, Class keyword)
    -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, keyword);

  ctx.require(keyword.get_type());
  stmt->cond = parse_required_pack(
      ctx, "If statements take a condition pack, e.g. 'if (ready)'"_view);
  stmt->body = parse_func_body(ctx);

  if (ctx.matches(Class::Type::Else)) {
    ctx.advance();

    if (ctx.matches(Class::Type::If)) {
      Ast::Statement* else_if = parse_if_like(ctx, Class{Class::Type::If});
      Managed::Vector<Ast::Statement*> else_body(ctx.get_arena());
      else_body.insert(else_if);
      stmt->else_body = else_body.get_view();
    } else {
      stmt->else_body = parse_func_body(ctx);
    }
  }

  return stmt;
}

auto Ast::Statement::Parser::parse_if(Context& ctx) -> Ast::Statement* {
  return parse_if_like(ctx, Class::Type::If);
}

auto Ast::Statement::Parser::parse_for(Context& ctx) -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, Class::Type::For);

  ctx.require(Class::Type::For);
  stmt->layout = Ast::Layout::parse(ctx);
  ctx.require(Class::Type::In);
  stmt->cond = Expression::Expression::parse(ctx);
  stmt->body = parse_func_body(ctx);
  return stmt;
}

auto Ast::Statement::Parser::parse_while(Context& ctx) -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, Class::Type::While);

  ctx.require(Class::Type::While);
  stmt->cond = parse_required_pack(
      ctx, "While statements take a condition pack, e.g. 'while (ready)'"_view);
  stmt->body = parse_func_body(ctx);
  return stmt;
}

auto Ast::Statement::Parser::parse_match(Context& ctx) -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, Class::Type::Match);

  ctx.require(Class::Type::Match);
  stmt->cond = Expression::Expression::parse(ctx);
  ctx.require(Class::Type::ScopeStart);

  Managed::Vector<Ast::MatchCase> case_list(ctx.get_arena());

  while (ctx.matches(Class::Type::Case) && !ctx.is_done()) {
    ctx.advance();  // consume 'case'

    Ast::MatchCase arm;

    if (ctx.matches(Class::Type::Discard)) {
      arm.pattern = nullptr;
      ctx.advance();
    } else {
      arm.pattern = Expression::Expression::parse(ctx);
    }

    ctx.require(Class::Type::Define);
    arm.body = parse_func_body(ctx);
    case_list.insert(arm);
  }

  ctx.require(Class::Type::ScopeEnd);
  stmt->cases = case_list.get_view();
  return stmt;
}

auto Ast::Statement::Parser::parse_return(Context& ctx) -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, Class::Type::Return);

  ctx.require(Class::Type::Return);

  if (!ctx.matches(Class::Type::EndStatement)) {
    stmt->value = Expression::Expression::parse(ctx);
  }

  ctx.require(Class::Type::EndStatement);
  return stmt;
}

auto Ast::Statement::Parser::parse_break(Context& ctx) -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, Class::Type::Break);

  ctx.require(Class::Type::Break);
  ctx.require(Class::Type::EndStatement);
  return stmt;
}

auto Ast::Statement::Parser::parse_continue(Context& ctx) -> Ast::Statement* {
  Ast::Statement* stmt = alloc(ctx, Class::Type::Continue);

  ctx.require(Class::Type::Continue);
  ctx.require(Class::Type::EndStatement);
  return stmt;
}

auto Ast::Statement::Parser::matches_assignment_op(Class type) -> Bool {
  switch (type.get_type()) {
  case Class::Type::Assign:
  case Class::Type::AddAssign:
  case Class::Type::SubAssign:
    return True;

  default:
    return False;
  }
}

auto Ast::Statement::Parser::parse_declaration(Context& ctx)
    -> Ast::Statement* {
  Class sigil = ctx.get_current().get_class();
  ctx.advance();

  if (!ctx.matches(Class::Type::Addressable) &&
      !ctx.matches(Class::Type::Type)) {
    ctx.error("Expected a variable name after sigil"_view);
    return alloc(ctx, Class::Type::EndStatement);
  }

  View::Bytes var_name = ctx.get_current().get_text();
  ctx.advance();

  Ast::Statement* stmt = alloc(ctx, sigil);
  stmt->sigil = sigil;
  stmt->operator_class = Class::Type::Define;
  stmt->name = var_name;
  ctx.require(Class::Type::Define);
  stmt->type_ref = Type::Ref::parse(ctx);

  if (ctx.matches(Class::Type::Assign)) {
    ctx.advance();
    stmt->value = Expression::Expression::parse(ctx);
  }

  ctx.require(Class::Type::EndStatement);
  return stmt;
}

auto Ast::Statement::Parser::parse_scope_statement(Context& ctx)
    -> Ast::Statement* {
  switch (ctx.get_current().get_class().get_type()) {
  case Class::Type::If:
    return parse_if(ctx);

  case Class::Type::CompileIf:
    return parse_if_like(ctx, Class::Type::CompileIf);

  case Class::Type::For:
    return parse_for(ctx);

  case Class::Type::While:
    return parse_while(ctx);

  case Class::Type::Match:
    return parse_match(ctx);

  default:
    ctx.error("Expected a scope statement keyword"_view);
    return alloc(ctx, Class::Type::EndStatement);
  }
}

auto Ast::Statement::Parser::is_assignable_root(
    const Expression::Expression* expr) -> Bool {
  if (!expr) {
    return False;
  }

  switch (expr->get_class().get_type()) {
  case Class::Type::Addressable:
  case Class::Type::Self:
  case Class::Type::Package:
    return True;

  default:
    return is_assignable_base(expr);
  }
}

auto Ast::Statement::Parser::is_assignable_base(
    const Expression::Expression* expr) -> Bool {
  if (!expr) {
    return False;
  }

  switch (expr->get_class().get_type()) {
  case Class::Type::AddressOp:
  case Class::Type::IndexStart:
  case Class::Type::SwizzleOp:
  case Class::Type::SliceOp:
    return is_assignable_root(expr->get_left());

  default:
    return False;
  }
}

auto Ast::Statement::Parser::is_assignable_address(
    const Expression::Expression* expr) -> Bool {
  if (!expr) {
    return False;
  }

  if (expr->get_class() == Class::Type::Addressable) {
    return True;
  }

  return is_assignable_base(expr);
}

auto Ast::Statement::Parser::parse_assignment_or_expr_statement(Context& ctx)
    -> Ast::Statement* {
  Expression::Expression* left_expression = Expression::Expression::parse(ctx);
  Class assignment_operator = ctx.get_current().get_class();

  if (matches_assignment_op(assignment_operator)) {
    if (!is_assignable_address(left_expression)) {
      ctx.error(
          "Expected an assignable address"_view,
          "Assignments must target an addressable access chain"_view);
    }

    Ast::Statement* stmt = alloc(ctx, assignment_operator);
    stmt->operator_class = assignment_operator;
    stmt->cond = left_expression;
    ctx.advance();
    stmt->value = Expression::Expression::parse(ctx);
    ctx.require(Class::Type::EndStatement);
    return stmt;
  }

  // Expression statement (function call as a statement, etc.)
  Ast::Statement* stmt = alloc(ctx, Class::Type::EndStatement);
  stmt->value = left_expression;
  ctx.require(Class::Type::EndStatement);
  return stmt;
}

auto Ast::Statement::Parser::parse(Context& ctx) -> Statement* {
  while (ctx.matches(Class::Type::Comment)) {
    ctx.advance();
  }

  if (ctx.matches(Class::Type::ScopeEnd) || ctx.is_done()) {
    return nullptr;
  }

  Class statement_start = ctx.get_current().get_class();

  if (statement_start.is_sigil()) {
    return parse_declaration(ctx);
  }

  switch (statement_start.get_type()) {
  case Class::Type::Return:
    return parse_return(ctx);

  case Class::Type::Break:
    return parse_break(ctx);

  case Class::Type::Continue:
    return parse_continue(ctx);

  case Class::Type::If:
  case Class::Type::CompileIf:
  case Class::Type::For:
  case Class::Type::While:
  case Class::Type::Match:
    return parse_scope_statement(ctx);

  default:
    return parse_assignment_or_expr_statement(ctx);
  }
}

auto Ast::Statement::parse(Context& ctx) -> Statement* {
  return Parser::parse(ctx);
}
