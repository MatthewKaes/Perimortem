// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/expression.hpp"

#include "tetrodotoxin/syntax/expression/access.hpp"
#include "tetrodotoxin/syntax/expression/addressable.hpp"
#include "tetrodotoxin/syntax/expression/binary.hpp"
#include "tetrodotoxin/syntax/expression/data.hpp"
#include "tetrodotoxin/syntax/expression/literal.hpp"
#include "tetrodotoxin/syntax/expression/type_access.hpp"
#include "tetrodotoxin/syntax/expression/unary.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax::Expression;

auto Tetrodotoxin::Format::format(Formatter& ctx, const ExpressionNode* expr)
    -> void {
  if (!expr) {
    return;
  }

  switch (expr->get_kind()) {
  case Kind::Literal:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Literal*>(expr));
    return;

  case Kind::Addressable:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Addressable*>(expr));
    return;

  case Kind::TypeAccess:
    Tetrodotoxin::Format::format(ctx, *static_cast<const TypeAccess*>(expr));
    return;

  case Kind::Data:
    Tetrodotoxin::Format::format(
        ctx, *static_cast<const Tetrodotoxin::Syntax::Expression::Data*>(expr));
    return;

  case Kind::Unary:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Unary*>(expr));
    return;

  case Kind::Binary:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Binary*>(expr));
    return;

  case Kind::FieldAccess:
    Tetrodotoxin::Format::format(ctx, *static_cast<const FieldAccess*>(expr));
    return;

  case Kind::Call:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Call*>(expr));
    return;

  case Kind::Index:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Index*>(expr));
    return;

  case Kind::IndexSlice:
    Tetrodotoxin::Format::format(ctx, *static_cast<const IndexSlice*>(expr));
    return;

  case Kind::Swizzle:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Swizzle*>(expr));
    return;

  case Kind::Slice:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Slice*>(expr));
    return;

  case Kind::Pack:
    Tetrodotoxin::Format::format(ctx, *static_cast<const Pack*>(expr));
    return;

  case Kind::Expression:
    ctx << expr->get_text();
    return;
  }
}

auto Tetrodotoxin::Format::measure(const ExpressionNode* expr) -> Count {
  if (!expr) {
    return 0;
  }

  switch (expr->get_kind()) {
  case Kind::Literal:
    return Tetrodotoxin::Format::measure(*static_cast<const Literal*>(expr));

  case Kind::Addressable:
    return Tetrodotoxin::Format::measure(
        *static_cast<const Addressable*>(expr));

  case Kind::TypeAccess:
    return Tetrodotoxin::Format::measure(*static_cast<const TypeAccess*>(expr));

  case Kind::Data:
    return Tetrodotoxin::Format::measure(
        *static_cast<const Tetrodotoxin::Syntax::Expression::Data*>(expr));

  case Kind::Unary:
    return Tetrodotoxin::Format::measure(*static_cast<const Unary*>(expr));

  case Kind::Binary:
    return Tetrodotoxin::Format::measure(*static_cast<const Binary*>(expr));

  case Kind::FieldAccess:
    return Tetrodotoxin::Format::measure(
        *static_cast<const FieldAccess*>(expr));

  case Kind::Call:
    return Tetrodotoxin::Format::measure(*static_cast<const Call*>(expr));

  case Kind::Index:
    return Tetrodotoxin::Format::measure(*static_cast<const Index*>(expr));

  case Kind::IndexSlice:
    return Tetrodotoxin::Format::measure(*static_cast<const IndexSlice*>(expr));

  case Kind::Swizzle:
    return Tetrodotoxin::Format::measure(*static_cast<const Swizzle*>(expr));

  case Kind::Slice:
    return Tetrodotoxin::Format::measure(*static_cast<const Slice*>(expr));

  case Kind::Pack:
    return Tetrodotoxin::Format::measure(*static_cast<const Pack*>(expr));

  case Kind::Expression:
    return expr->get_text().get_size();
  }

  return 0;
}
