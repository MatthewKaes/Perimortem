// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/binary.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

static auto measure_expr(const Expression::Expression* expr) -> Count {
  return Tetrodotoxin::Format::measure(expr);
}

static auto is_breakable_binary_op(Class operator_class) -> Bool {
  switch (operator_class.get_type()) {
  case Class::Type::And:
  case Class::Type::Or:
  case Class::Type::CmpOp:
  case Class::Type::NotEqOp:
  case Class::Type::LessOp:
  case Class::Type::GreaterOp:
  case Class::Type::LessEqOp:
  case Class::Type::GreaterEqOp:
    return True;

  default:
    return False;
  }
}

static auto is_binary_expr(const Expression::Expression* expr) -> Bool {
  if (!expr) {
    return False;
  }

  switch (expr->get_class().get_type()) {
  case Class::Type::SubOp:
  case Class::Type::AddOp:
  case Class::Type::MulOp:
  case Class::Type::DivOp:
  case Class::Type::ModOp:
  case Class::Type::CmpOp:
  case Class::Type::NotEqOp:
  case Class::Type::LessOp:
  case Class::Type::GreaterOp:
  case Class::Type::LessEqOp:
  case Class::Type::GreaterEqOp:
  case Class::Type::And:
  case Class::Type::Or:
  case Class::Type::RangeOp:
    return True;

  default:
    return False;
  }
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::Binary& expr) -> void {
  switch (expr.get_class().get_type()) {
  case Class::Type::SubOp:
  case Class::Type::AddOp:
  case Class::Type::MulOp:
  case Class::Type::DivOp:
  case Class::Type::ModOp:
  case Class::Type::CmpOp:
  case Class::Type::NotEqOp:
  case Class::Type::LessOp:
  case Class::Type::GreaterOp:
  case Class::Type::LessEqOp:
  case Class::Type::GreaterEqOp:
  case Class::Type::And:
  case Class::Type::Or: {
    Bool parenthesize_left =
        is_binary_expr(expr.get_left()) &&
        Expression::Binary::precedence(expr.get_left()->get_class()) <
            Expression::Binary::precedence(expr.get_class());
    Bool parenthesize_right =
        is_binary_expr(expr.get_right()) &&
        Expression::Binary::precedence(expr.get_right()->get_class()) <
            Expression::Binary::precedence(expr.get_class());
    Bool break_line =
        is_breakable_binary_op(expr.get_class()) &&
        ctx.current_line_width() + Tetrodotoxin::Format::measure(expr) >
            Tetrodotoxin::Format::soft_line_limit;

    if (parenthesize_left) {
      ctx << Class::Type::PackingStart;
    }

    if (expr.get_left()) {
      Tetrodotoxin::Format::format(ctx, expr.get_left());
    }

    if (parenthesize_left) {
      ctx << Class::Type::PackingEnd;
    }

    if (break_line) {
      ctx.emit_newline();
      ctx.indent++;
      ctx.do_indent();
      ctx << expr.get_class().get_source_text();
      ctx << " "_view;

      if (parenthesize_right) {
        ctx << Class::Type::PackingStart;
      }

      if (expr.get_right()) {
        Tetrodotoxin::Format::format(ctx, expr.get_right());
      }

      if (parenthesize_right) {
        ctx << Class::Type::PackingEnd;
      }

      ctx.indent--;
      break;
    }

    ctx << " "_view;
    ctx << expr.get_class().get_source_text();
    ctx << " "_view;

    if (parenthesize_right) {
      ctx << Class::Type::PackingStart;
    }

    if (expr.get_right()) {
      Tetrodotoxin::Format::format(ctx, expr.get_right());
    }

    if (parenthesize_right) {
      ctx << Class::Type::PackingEnd;
    }
    break;
  }

  case Class::Type::RangeOp:
    if (expr.get_left()) {
      Tetrodotoxin::Format::format(ctx, expr.get_left());
    }
    ctx << Class::Type::RangeOp;
    if (expr.get_right()) {
      Tetrodotoxin::Format::format(ctx, expr.get_right());
    }
    break;

  default:
    ctx << expr.get_text();
    break;
  }
}

auto Tetrodotoxin::Format::measure(const Expression::Binary& expr) -> Count {
  switch (expr.get_class().get_type()) {
  case Class::Type::SubOp:
  case Class::Type::AddOp:
  case Class::Type::MulOp:
  case Class::Type::DivOp:
  case Class::Type::ModOp:
  case Class::Type::CmpOp:
  case Class::Type::NotEqOp:
  case Class::Type::LessOp:
  case Class::Type::GreaterOp:
  case Class::Type::LessEqOp:
  case Class::Type::GreaterEqOp:
  case Class::Type::And:
  case Class::Type::Or:
    return measure_expr(expr.get_left()) + 2 +
           expr.get_class().get_source_text().get_size() +
           measure_expr(expr.get_right());

  case Class::Type::RangeOp:
    return measure_expr(expr.get_left()) +
           Class::get_source_text(Class::Type::RangeOp).get_size() +
           measure_expr(expr.get_right());

  default:
    return expr.get_text().get_size();
  }
}
