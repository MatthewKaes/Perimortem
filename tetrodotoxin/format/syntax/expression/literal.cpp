// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/literal.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

static auto format_wrapped_literal(Formatter& ctx, View::Bytes text, Class type)
    -> void {
  View::Bytes open = type.get_source_text();
  View::Bytes close = Class::get_source_text(Class::Type::IndexEnd);

  if (text.get_size() < open.get_size() + close.get_size()) {
    ctx << text;
    return;
  }

  if (text.slice(0, open.get_size()) != open ||
      text.slice(text.get_size() - close.get_size()) != close) {
    ctx << text;
    return;
  }

  ctx << type;
  ctx << text.slice(
      open.get_size(), text.get_size() - open.get_size() - close.get_size());
  ctx << Class::Type::IndexEnd;
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::Literal& expr) -> void {
  switch (expr.get_class().get_type()) {
  case Class::Type::Bytes:
  case Class::Type::Embedded:
    format_wrapped_literal(ctx, expr.get_text(), expr.get_class());
    return;

  default:
    break;
  }

  ctx << expr.get_text();
}

auto Tetrodotoxin::Format::measure(const Expression::Literal& expr) -> Count {
  return expr.get_text().get_size();
}
