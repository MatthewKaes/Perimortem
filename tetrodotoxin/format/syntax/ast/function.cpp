// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format_declaration(
    Formatter& ctx,
    const Ast::Function& function,
    Bool first) -> void {
  const Ast::Comment& comment = function.get_documentation();
  Bool has_comment = !comment.is_empty();

  if (has_comment && !first) {
    ctx.ensure_blank();
  }

  if (has_comment) {
    Tetrodotoxin::Format::format(ctx, comment);
  }

  if (function.is_disabled()) {
    ctx.do_indent();
    ctx << Class::Type::Disabled << " "_view;
  }

  View::Vector<Ast::Attribute> attrs = function.get_attributes();

  for (Count i = 0; i < attrs.get_size(); i++) {
    if (!function.is_disabled() || i != 0) {
      ctx.do_indent();
    }

    Tetrodotoxin::Format::format(ctx, attrs[i]);
    ctx.emit_newline();
  }

  ctx.do_indent();
  if (function.is_external()) {
    ctx << Class{Class::Type::External}.get_source_text();
    ctx << " "_view;
  }

  View::Bytes sigil_text =
      function.get_definition().get_sigil().get_source_text();

  if (!sigil_text.is_empty()) {
    ctx << sigil_text;
    ctx << " "_view;
  }

  ctx << Class::Type::Func << " "_view;
  ctx << function.get_definition().get_name();
  Tetrodotoxin::Format::format(ctx, function);
  ctx.emit_newline();
}

auto Tetrodotoxin::Format::format_block(
    Formatter& ctx,
    View::Vector<Ast::Function*> functions) -> void {
  Bool first = True;

  for (Count i = 0; i < functions.get_size(); i++) {
    if (!functions[i]) {
      continue;
    }

    if (!first && functions[i]->get_documentation().is_empty()) {
      ctx.ensure_blank();
    }

    Tetrodotoxin::Format::format_declaration(ctx, *functions[i], first);
    first = False;
  }
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Function& function)
    -> void {
  View::Vector<Ast::Param> params = function.get_params().get_params();
  ctx << Class::Type::IndexStart;

  if (params.get_size() <= 2) {
    for (Count i = 0; i < params.get_size(); i++) {
      if (i > 0) {
        ctx << Class::Type::PackingOp << " "_view;
      }

      Tetrodotoxin::Format::format(ctx, params[i]);
    }
  } else {
    ctx.emit_newline();
    ctx.indent++;

    for (Count i = 0; i < params.get_size(); i++) {
      ctx.do_indent();
      Tetrodotoxin::Format::format(ctx, params[i]);
      ctx << Class::Type::PackingOp;
      ctx.emit_newline();
    }

    ctx.indent--;
    ctx.do_indent();
  }

  ctx << Class::Type::IndexEnd << " "_view << Class::Type::CallOp << " "_view;
  Tetrodotoxin::Format::format(ctx, function.get_returns());

  if (!function.has_body()) {
    ctx << Class::Type::EndStatement;
    return;
  }

  ctx << " "_view << Class::Type::ScopeStart;
  ctx.emit_newline();
  ctx.indent++;
  Tetrodotoxin::Format::format_block(ctx, function.get_body());
  ctx.indent--;
  ctx.do_indent();
  ctx << Class::Type::ScopeEnd;
}
