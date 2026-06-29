// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/unary.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

static auto measure_expr(const Expression::Expression* expr) -> Count {
  return Tetrodotoxin::Format::measure(expr);
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Expression::Unary& expr)
    -> void {
  switch (expr.get_class().get_type()) {
  case Class::Type::NotOp:
    ctx << Class::Type::NotOp;
    break;

  case Class::Type::SubOp:
    ctx << Class::Type::SubOp;
    break;

  default:
    ctx << expr.get_text();
    return;
  }

  if (expr.get_right()) {
    Tetrodotoxin::Format::format(ctx, expr.get_right());
  }
}

auto Tetrodotoxin::Format::measure(const Expression::Unary& expr) -> Count {
  switch (expr.get_class().get_type()) {
  case Class::Type::NotOp:
  case Class::Type::SubOp:
    return expr.get_class().get_source_text().get_size() +
           measure_expr(expr.get_right());

  default:
    return expr.get_text().get_size();
  }
}
