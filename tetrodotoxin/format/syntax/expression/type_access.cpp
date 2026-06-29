// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/type_access.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::TypeAccess& expr) -> void {
  Tetrodotoxin::Format::format(ctx, expr.get_type_ref());
}

auto Tetrodotoxin::Format::measure(const Expression::TypeAccess& expr)
    -> Count {
  return Tetrodotoxin::Format::measure(expr.get_type_ref());
}
