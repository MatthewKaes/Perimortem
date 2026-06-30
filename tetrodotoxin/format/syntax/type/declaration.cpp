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

static auto measure_enum_value_names(View::Vector<Ast::Member*> members)
    -> Count {
  Count width = 0;
  for (Count i = 0; i < members.get_size(); i++) {
    if (!members[i]) {
      continue;
    }

    Count name_width = members[i]->get_definition().get_name().get_size();
    if (name_width > width) {
      width = name_width;
    }
  }

  return width;
}

static auto format_enum_value(
    Formatter& ctx,
    const Ast::Member& member,
    Bool first,
    Count name_width) -> void {
  const Ast::Comment& comment = member.get_documentation();
  Bool has_comment = !comment.is_empty();

  if (has_comment && !first) {
    ctx.ensure_blank();
  }

  if (has_comment) {
    Tetrodotoxin::Format::format(ctx, comment);
  }

  if (member.is_disabled()) {
    ctx.do_indent();
    ctx << Class::Type::Disabled << " "_view;
  }

  View::Vector<Ast::Attribute> attrs = member.get_attributes();
  for (Count i = 0; i < attrs.get_size(); i++) {
    if (!member.is_disabled() || i != 0) {
      ctx.do_indent();
    }

    Tetrodotoxin::Format::format(ctx, attrs[i]);
    ctx.emit_newline();
  }

  const Ast::Definition& definition = member.get_definition();
  ctx.do_indent();
  ctx << definition.get_name();
  ctx.emit_padding(name_width, definition.get_name().get_size());
  ctx << " "_view << Class::Type::Assign << " "_view;
  Tetrodotoxin::Format::format(ctx, definition.get_value());
  ctx << Class::Type::EndStatement;
  ctx.emit_newline();
}

static auto format_enum_values(
    Formatter& ctx,
    View::Vector<Ast::Member*> members) -> void {
  Count name_width = measure_enum_value_names(members);
  Bool first = True;

  for (Count i = 0; i < members.get_size(); i++) {
    if (!members[i]) {
      continue;
    }

    format_enum_value(ctx, *members[i], first, name_width);
    first = False;
  }
}

static auto format_enum_body(Formatter& ctx, const Type::Body& body) -> void {
  Bool emitted = False;

  if (!body.get_members().is_empty()) {
    format_enum_values(ctx, body.get_members());
    emitted = True;
  }

  if (!body.get_functions().is_empty()) {
    if (emitted) {
      ctx.ensure_blank();
    }
    Tetrodotoxin::Format::format_block(ctx, body.get_functions());
    emitted = True;
  }

  if (!body.get_types().is_empty()) {
    if (emitted) {
      ctx.ensure_blank();
    }
    Tetrodotoxin::Format::format_block(ctx, body.get_types());
  }
}

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
  if (type.is_scoped_body()) {
    Tetrodotoxin::Format::format_header(
        ctx, type.get_definition(),
        type.get_definition().get_name().get_size());
    ctx << " "_view << Class::Type::ScopeStart;
    ctx.emit_newline();
    ctx.indent++;
    if (type.get_kind() == DeclarationKind::Enum) {
      format_enum_body(ctx, type.get_body());
    } else {
      Tetrodotoxin::Format::format(ctx, type.get_body());
    }
    ctx.indent--;
    ctx.do_indent();
    ctx << Class::Type::ScopeEnd;
  } else if (type.get_kind() == DeclarationKind::Enum) {
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
