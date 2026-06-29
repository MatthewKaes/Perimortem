// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/access.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

static auto measure_expr(const Expression::Expression* expr) -> Count {
  return Tetrodotoxin::Format::measure(expr);
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::FieldAccess& expr) -> void {
  if (expr.get_left()) {
    Tetrodotoxin::Format::format(ctx, expr.get_left());
  }
  ctx << Class::Type::AddressOp;
  ctx << expr.get_text();
}

auto Tetrodotoxin::Format::measure(const Expression::FieldAccess& expr)
    -> Count {
  return measure_expr(expr.get_left()) +
         Class::get_source_text(Class::Type::AddressOp).get_size() +
         expr.get_text().get_size();
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Expression::Call& expr)
    -> void {
  if (expr.get_left()) {
    Tetrodotoxin::Format::format(ctx, expr.get_left());
  }
  ctx << " "_view << Class::Type::CallOp << " "_view;
  ctx << expr.get_text();
  if (expr.get_right()) {
    Tetrodotoxin::Format::format(ctx, expr.get_right());
  } else {
    ctx << Class::Type::PackingStart << Class::Type::PackingEnd;
  }
}

auto Tetrodotoxin::Format::measure(const Expression::Call& expr) -> Count {
  Count args_width = measure_expr(expr.get_right());
  if (!expr.get_right()) {
    args_width = Class::get_source_text(Class::Type::PackingStart).get_size() +
                 Class::get_source_text(Class::Type::PackingEnd).get_size();
  }
  return measure_expr(expr.get_left()) + 1 +
         Class::get_source_text(Class::Type::CallOp).get_size() + 1 +
         expr.get_text().get_size() + args_width;
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Expression::Index& expr)
    -> void {
  if (expr.get_left()) {
    Tetrodotoxin::Format::format(ctx, expr.get_left());
  }
  ctx << Class::Type::IndexStart;
  if (expr.get_right()) {
    Tetrodotoxin::Format::format(ctx, expr.get_right());
  }
  ctx << Class::Type::IndexEnd;
}

auto Tetrodotoxin::Format::measure(const Expression::Index& expr) -> Count {
  return measure_expr(expr.get_left()) +
         Class::get_source_text(Class::Type::IndexStart).get_size() +
         measure_expr(expr.get_right()) +
         Class::get_source_text(Class::Type::IndexEnd).get_size();
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::IndexSlice& expr) -> void {
  if (expr.get_left()) {
    Tetrodotoxin::Format::format(ctx, expr.get_left());
  }

  ctx << Class::Type::IndexStart;
  if (expr.get_start()) {
    Tetrodotoxin::Format::format(ctx, expr.get_start());
  }
  ctx << " "_view << Class::Type::SliceOp << " "_view;
  if (expr.get_count()) {
    Tetrodotoxin::Format::format(ctx, expr.get_count());
  }
  ctx << Class::Type::IndexEnd;
}

auto Tetrodotoxin::Format::measure(const Expression::IndexSlice& expr)
    -> Count {
  return measure_expr(expr.get_left()) +
         Class::get_source_text(Class::Type::IndexStart).get_size() +
         measure_expr(expr.get_start()) + 1 +
         Class::get_source_text(Class::Type::SliceOp).get_size() + 1 +
         measure_expr(expr.get_count()) +
         Class::get_source_text(Class::Type::IndexEnd).get_size();
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::Swizzle& expr) -> void {
  if (expr.get_left()) {
    Tetrodotoxin::Format::format(ctx, expr.get_left());
  }
  ctx << Class::Type::SwizzleOp;

  View::Vector<Expression::Expression*> fields = expr.get_args();
  for (Count i = 0; i < fields.get_size(); i++) {
    if (i > 0) {
      ctx << Class::Type::PackingOp << " "_view;
    }
    if (fields[i]) {
      Tetrodotoxin::Format::format(ctx, fields[i]);
    }
  }

  ctx << Class::Type::IndexEnd;
}

auto Tetrodotoxin::Format::measure(const Expression::Swizzle& expr) -> Count {
  Count width = measure_expr(expr.get_left()) +
                Class::get_source_text(Class::Type::SwizzleOp).get_size();

  View::Vector<Expression::Expression*> fields = expr.get_args();
  for (Count i = 0; i < fields.get_size(); i++) {
    if (i > 0) {
      width += Class::get_source_text(Class::Type::PackingOp).get_size() + 1;
    }

    width += measure_expr(fields[i]);
  }

  return width + Class::get_source_text(Class::Type::IndexEnd).get_size();
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Expression::Slice& expr)
    -> void {
  if (expr.get_left()) {
    Tetrodotoxin::Format::format(ctx, expr.get_left());
  }
  ctx << Class::Type::SwizzleOp;
  if (expr.get_start()) {
    Tetrodotoxin::Format::format(ctx, expr.get_start());
  }
  ctx << " "_view << Class::Type::SliceOp << " "_view;
  if (expr.get_count()) {
    Tetrodotoxin::Format::format(ctx, expr.get_count());
  }
  ctx << Class::Type::IndexEnd;
}

auto Tetrodotoxin::Format::measure(const Expression::Slice& expr) -> Count {
  return measure_expr(expr.get_left()) +
         Class::get_source_text(Class::Type::SwizzleOp).get_size() +
         measure_expr(expr.get_start()) + 1 +
         Class::get_source_text(Class::Type::SliceOp).get_size() + 1 +
         measure_expr(expr.get_count()) +
         Class::get_source_text(Class::Type::IndexEnd).get_size();
}
