// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/syntax.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/ast/value.hpp"
#include "tetrodotoxin/syntax/type/body.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Type::Declaration& type,
    Bool first) -> void {
  const Ast::Comment& comment = type.get_documentation();
  Bool has_comment = !comment.is_empty();

  if (has_comment && !first) {
    ctx.ensure_blank();
  }

  if (has_comment) {
    Tetrodotoxin::Format::format(ctx, comment);
  }

  if (type.is_disabled()) {
    ctx.do_indent();
    ctx << Class::Type::Disabled << " "_view;
  }

  View::Vector<Ast::Attribute> attrs = type.get_attributes();

  for (Count i = 0; i < attrs.get_size(); i++) {
    if (!type.is_disabled() || i != 0) {
      ctx.do_indent();
    }

    Tetrodotoxin::Format::format(ctx, attrs[i]);
    ctx.emit_newline();
  }

  ctx.do_indent();
  if (type.is_scope()) {
    Tetrodotoxin::Format::format_header(
        ctx, type.get_definition(),
        type.get_definition().get_name().get_size());
    ctx << " "_view << Class::Type::ScopeStart;
    ctx.emit_newline();
    ctx.indent++;
    Tetrodotoxin::Format::format(ctx, type.get_body());
    ctx.indent--;
    ctx.do_indent();
    ctx << Class::Type::ScopeEnd;
  } else if (type.is_enum()) {
    Tetrodotoxin::Format::format_header(
        ctx, type.get_definition(),
        type.get_definition().get_name().get_size());
    ctx << Class::Type::PackingStart;

    View::Vector<Ast::Member*> values = type.get_scope();

    for (Count i = 0; i < values.get_size(); i++) {
      if (i > 0) {
        ctx << Class::Type::PackingOp << " "_view;
      }

      const Ast::Definition& definition = values[i]->get_definition();
      ctx << Class::Type::AddressOp;
      ctx << definition.get_name();
      ctx << " "_view << Class::Type::Assign << " "_view;
      Tetrodotoxin::Format::format(ctx, definition.get_value());
    }

    ctx << Class::Type::PackingEnd;
    ctx << Class::Type::EndStatement;
  }
  ctx.emit_newline();
}

auto Tetrodotoxin::Format::format_block(
    Formatter& ctx,
    View::Vector<Type::Declaration*> types) -> void {
  Bool first = True;

  for (Count i = 0; i < types.get_size(); i++) {
    if (!types[i]) {
      continue;
    }

    if (!first && types[i]->get_documentation().is_empty()) {
      ctx.ensure_blank();
    }

    Tetrodotoxin::Format::format(ctx, *types[i], first);
    first = False;
  }
}
