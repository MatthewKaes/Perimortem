// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/type/ref.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Type::Ref::parse(Context& ctx) -> Type::Ref {
  Type::Ref result;
  Managed::Vector<View::Bytes> path(ctx.get_arena());
  Managed::Vector<Type::Ref::Segment> segments(ctx.get_arena());

  // Numeric literals may appear as size params: Vec[Bits_8, 8]
  if (ctx.matches(Class::Type::Numeric)) {
    Type::Ref::Segment segment;
    segment.klass = ctx.get_current().get_class();
    segment.text = ctx.get_current().get_text();
    path.insert(segment.text);
    segments.insert(segment);
    ctx.advance();
    result.size_literal = True;
    result.name_path = path.get_view();
    result.segments = segments.get_view();
    return result;
  }

  if (!ctx.get_current().get_class().is_type_ref()) {
    ctx.error(
        "Expected a type name"_view,
        "Type names begin with an uppercase letter, e.g. Bits_32 or Vec[T, N]"_view);
    return result;
  }

  Type::Ref::Segment segment;
  segment.klass = ctx.get_current().get_class();
  segment.text = ctx.get_current().get_text();
  path.insert(segment.text);
  segments.insert(segment);
  ctx.advance();

  // Qualified path: Graphics.Image — only when inside generic params.
  // A bare AddressOp at expression level is a field access, not a type path,
  // so we only consume dot-segments when followed by another Type token.
  while (ctx.matches(Class::Type::AddressOp)) {
    if (!ctx.get_peek().get_class().is_type_ref()) {
      break;
    }

    ctx.advance();  // consume '.'
    Type::Ref::Segment next;
    next.klass = ctx.get_current().get_class();
    next.text = ctx.get_current().get_text();
    path.insert(next.text);
    segments.insert(next);
    ctx.advance();
  }

  result.name_path = path.get_view();
  result.segments = segments.get_view();

  // Generic params: Vec[T, N]
  if (!ctx.matches(Class::Type::IndexStart)) {
    return result;
  }

  ctx.advance();  // consume '['

  Managed::Vector<Type::Ref> type_params(ctx.get_arena());

  while (!ctx.matches(Class::Type::IndexEnd) && !ctx.is_done()) {
    type_params.insert(Type::Ref::parse(ctx));

    if (!ctx.matches(Class::Type::PackingOp)) {
      break;
    }

    ctx.advance();  // consume ','
  }

  ctx.require(Class::Type::IndexEnd);
  result.params = type_params.get_view();
  return result;
}
