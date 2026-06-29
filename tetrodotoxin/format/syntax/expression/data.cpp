// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/data.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Expression::Data& expr)
    -> void {
  ctx << Class::Type::ScopeStart;
  Tetrodotoxin::Format::format_args_after_open(
      ctx, expr.get_args(), Class::get_source_text(Class::Type::ScopeEnd),
      True);
}

auto Tetrodotoxin::Format::measure(const Expression::Data& expr) -> Count {
  Count width = Class::get_source_text(Class::Type::ScopeStart).get_size() +
                Class::get_source_text(Class::Type::ScopeEnd).get_size() +
                Tetrodotoxin::Format::measure_args(expr.get_args());

  if (!expr.get_args().is_empty()) {
    width += 2;
  }

  return width;
}
