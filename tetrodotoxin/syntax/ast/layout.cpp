// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/layout.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto starts_named_layout_field(Class::Type type) -> Bool {
  return type == Class::Type::AddressOp || type == Class::Type::Attribute;
}

auto Ast::Layout::parse(Context& ctx) -> Layout {
  Layout result;

  if (!ctx.matches(Class::Type::IndexStart)) {
    if (ctx.matches(Class::Type::PackingStart)) {
      ctx.advance();
      Managed::Vector<Param> param_list(ctx.get_arena());

      while (!ctx.matches(Class::Type::PackingEnd) && !ctx.is_done()) {
        param_list.insert(Param::parse(ctx));

        if (!ctx.matches(Class::Type::PackingOp)) {
          break;
        }

        ctx.advance();
      }

      ctx.require(Class::Type::PackingEnd);
      result.params = param_list.get_view();
      return result;
    }

    result.value_type = Type::Ref::parse(ctx);
    return result;
  }

  Class::Type first = ctx.get_peek(1).get_class().get_type();

  if (starts_named_layout_field(first)) {
    ctx.advance();  // consume '['
    Managed::Vector<Param> param_list(ctx.get_arena());

    while (!ctx.matches(Class::Type::IndexEnd) && !ctx.is_done()) {
      param_list.insert(Param::parse(ctx));

      if (!ctx.matches(Class::Type::PackingOp)) {
        break;
      }

      ctx.advance();  // consume ','
    }

    ctx.require(Class::Type::IndexEnd);
    result.params = param_list.get_view();
    return result;
  }

  if (first == Class::Type::IndexEnd) {
    ctx.advance();  // consume '['
    ctx.advance();  // consume ']'
    return result;
  }

  // Unnamed tuple: parse Type::Ref list, decide Value vs Unnamed after.
  ctx.advance();  // consume '['
  Managed::Vector<Type::Ref> type_list(ctx.get_arena());

  while (!ctx.matches(Class::Type::IndexEnd) && !ctx.is_done()) {
    type_list.insert(Type::Ref::parse(ctx));

    if (!ctx.matches(Class::Type::PackingOp)) {
      break;
    }

    ctx.advance();  // consume ','
  }

  ctx.require(Class::Type::IndexEnd);

  if (type_list.get_size() == 1) {
    result.value_type = type_list[0];
  } else {
    result.types = type_list.get_view();
  }

  return result;
}
