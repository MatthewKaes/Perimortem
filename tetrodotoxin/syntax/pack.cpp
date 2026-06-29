// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

enum class PackFieldMode : Bits_8 {
  Undecided,
  Named,
  Indexed,
  Positional,
};

static auto report_mixed_pack_fields(Context& ctx) -> void {
  ctx.error(
      "Cannot mix named and positional pack fields"_view,
      "Use either '(.field = value, ...)' or '(value, ...)' for the "
      "whole pack"_view);
}

static auto starts_designated_pack_field(Context& ctx) -> Bool {
  return ctx.matches(Class::Type::AddressOp);
}

static auto recover_pack_field(Context& ctx) -> void {
  Count packing_depth = 0;
  Count index_depth = 0;
  Count scope_depth = 0;

  while (!ctx.is_done()) {
    Class::Type current = ctx.get_current().get_class().get_type();
    if (packing_depth == 0 && index_depth == 0 && scope_depth == 0) {
      switch (current) {
      case Class::Type::PackingOp:
      case Class::Type::PackingEnd:
        return;

      default:
        break;
      }
    }

    switch (current) {
    case Class::Type::PackingStart:
      packing_depth++;
      break;

    case Class::Type::PackingEnd:
      if (packing_depth == 0) {
        return;
      }
      packing_depth--;
      break;

    case Class::Type::IndexStart:
      index_depth++;
      break;

    case Class::Type::IndexEnd:
      if (index_depth > 0) {
        index_depth--;
      }
      break;

    case Class::Type::ScopeStart:
      scope_depth++;
      break;

    case Class::Type::ScopeEnd:
      if (scope_depth > 0) {
        scope_depth--;
      }
      break;

    default:
      break;
    }

    ctx.advance();
  }
}

Pack::Pack(Context& ctx, View::Vector<Field> fields)
    : ExpressionNode(Class::Type::PackingStart), fields(fields) {
  Managed::Vector<ExpressionNode*> values(ctx.get_arena());

  for (Count i = 0; i < fields.get_size(); i++) {
    values.insert(fields[i].value);
  }

  this->values = values.get_view();
}

static auto parse_designated_pack_field(Context& ctx) -> Pack::Field {
  Pack::Field field;
  ctx.advance();

  switch (ctx.get_current().get_class().get_type()) {
  case Class::Type::Addressable:
    field.klass = ctx.get_current().get_class();
    field.name = ctx.get_current().get_text();
    ctx.advance();
    break;

  case Class::Type::Numeric:
    field.klass = ctx.get_current().get_class();
    field.index = ctx.get_current().get_text();
    ctx.advance();
    break;

  default:
    ctx.error(
        "Expected a field name or index after '.'"_view,
        "Use '.snake_case = value' for named fields or '.10 = value' "
        "for indexed table fields"_view);
    recover_pack_field(ctx);
    return field;
  }

  if (!ctx.require(Class::Type::Assign)) {
    recover_pack_field(ctx);
    return field;
  }

  field.value = ExpressionNode::parse(ctx);
  return field;
}

auto Pack::parse(Context& ctx) -> Pack* {
  ctx.require(Class::Type::PackingStart);

  if (ctx.matches(Class::Type::PackingEnd)) {
    ctx.advance();
    return ExpressionNode::construct<Pack>(ctx, ctx, View::Vector<Field>());
  }

  Pack* pack = ExpressionNode::construct<Pack>(ctx, ctx, parse_fields(ctx));
  ctx.require(Class::Type::PackingEnd);
  return pack;
}

auto Pack::parse_fields(Context& ctx) -> View::Vector<Field> {
  Managed::Vector<Field> elems(ctx.get_arena());
  PackFieldMode mode = PackFieldMode::Undecided;

  while (!ctx.matches(Class::Type::PackingEnd) && !ctx.is_done()) {
    Field field;

    if (starts_designated_pack_field(ctx)) {
      if (mode == PackFieldMode::Positional) {
        report_mixed_pack_fields(ctx);
      }

      Field designated = parse_designated_pack_field(ctx);
      PackFieldMode designated_mode = !designated.index.is_empty()
                                          ? PackFieldMode::Indexed
                                          : PackFieldMode::Named;
      if (mode != PackFieldMode::Undecided && mode != designated_mode) {
        report_mixed_pack_fields(ctx);
      }

      mode = designated_mode;
      elems.insert(designated);
    } else {
      if (mode == PackFieldMode::Named || mode == PackFieldMode::Indexed) {
        report_mixed_pack_fields(ctx);
      }

      mode = PackFieldMode::Positional;
      field.value = ExpressionNode::parse(ctx);
      elems.insert(field);
    }

    switch (ctx.get_current().get_class().get_type()) {
    case Class::Type::PackingOp:
      ctx.advance();
      break;

    default:
      return elems.get_view();
    }
  }

  return elems.get_view();
}

auto Pack::has_named_field(View::Vector<Field> fields) -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (!fields[i].name.is_empty()) {
      return True;
    }
  }

  return False;
}

auto Pack::has_indexed_field(View::Vector<Field> fields) -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (!fields[i].index.is_empty()) {
      return True;
    }
  }

  return False;
}
