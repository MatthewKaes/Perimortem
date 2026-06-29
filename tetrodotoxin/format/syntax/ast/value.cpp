// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Value& value)
    -> void {
  if (value.get_expr()) {
    Tetrodotoxin::Format::format(ctx, value.get_expr());
  }
}

auto Tetrodotoxin::Format::measure(const Ast::Value& value) -> Count {
  return Tetrodotoxin::Format::measure(value.get_expr());
}
